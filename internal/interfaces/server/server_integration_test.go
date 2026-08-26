package server_test

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"
	"net/http/cookiejar"
	"net/http/httptest"
	"path/filepath"
	"testing"
	"time"

	"github.com/box1o/woki/internal/application"
	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/config"
)

func TestBrowserDeviceAndWorkspaceFlow(t *testing.T) {
	dataFile := filepath.Join(t.TempDir(), "woki.json")
	cfg := config.Config{
		Environment: config.Development,
		Version:     "test",
		Server: config.ServerConfig{
			Addr:              ":0",
			ReadHeaderTimeout: time.Second,
			ReadTimeout:       5 * time.Second,
			WriteTimeout:      5 * time.Second,
			IdleTimeout:       5 * time.Second,
			MaxHeaderBytes:    1 << 20,
		},
		Frontend:   config.FrontendConfig{URL: "http://frontend.test"},
		CORS:       config.CORSConfig{AllowedOrigin: "http://frontend.test"},
		Auth:       config.AuthConfig{Dev: true, Cookie: config.CookieConfig{Name: "woki_session", SameSite: "strict"}},
		Session:    config.SessionConfig{TTL: time.Hour},
		DeviceAuth: config.DeviceAuthConfig{CodeTTL: time.Minute, CredentialTTL: time.Hour},
		Storage:    config.StorageConfig{Backend: "file", DataFile: dataFile},
		Shutdown:   config.ShutdownConfig{Timeout: time.Second},
	}
	app, err := application.New(cfg)
	if err != nil {
		t.Fatal(err)
	}
	ts := httptest.NewServer(app.Handler())
	defer ts.Close()
	jar, _ := cookiejar.New(nil)
	browser := &http.Client{Jar: jar}
	postJSON(t, browser, ts.URL+"/auth/dev", map[string]string{"email": "owner@example.com", "name": "Owner"}, http.StatusOK, nil)
	var workspaces []api.Workspace
	getJSON(t, browser, ts.URL+"/workspaces", http.StatusOK, &workspaces)
	if len(workspaces) != 1 || workspaces[0].Name != "personal" {
		t.Fatalf("default workspaces=%+v", workspaces)
	}
	var created api.Workspace
	postJSON(t, browser, ts.URL+"/workspaces", map[string]string{"name": "engineering"}, http.StatusCreated, &created)

	memberJar, _ := cookiejar.New(nil)
	memberBrowser := &http.Client{Jar: memberJar}
	postJSON(
		t,
		memberBrowser,
		ts.URL+"/auth/dev",
		map[string]string{"email": "member@example.com", "name": "Member"},
		http.StatusOK,
		nil,
	)
	var added api.Member
	postJSON(
		t,
		browser,
		ts.URL+"/workspaces/"+created.ID+"/members",
		map[string]string{"email": "member@example.com", "role": "member"},
		http.StatusCreated,
		&added,
	)
	var promoted api.Member
	patchJSON(
		t,
		browser,
		ts.URL+"/workspaces/"+created.ID+"/members/"+added.ID,
		map[string]string{"role": "admin"},
		http.StatusOK,
		&promoted,
	)
	if promoted.Role != "admin" {
		t.Fatalf("promoted role=%q", promoted.Role)
	}
	requestStatus(
		t,
		memberBrowser,
		http.MethodDelete,
		ts.URL+"/workspaces/"+created.ID,
		http.StatusForbidden,
	)
	var code api.DeviceCodeResponse
	postJSON(t, http.DefaultClient, ts.URL+"/auth/device/code", map[string]string{"client_name": "integration CLI"}, http.StatusOK, &code)
	var request api.DeviceRequest
	getJSON(t, browser, ts.URL+"/auth/device/request?code="+code.UserCode, http.StatusOK, &request)
	postJSON(t, browser, ts.URL+"/auth/device/approve", map[string]string{"user_code": code.UserCode}, http.StatusNoContent, nil)
	var token api.DeviceTokenResponse
	postJSON(t, http.DefaultClient, ts.URL+"/auth/device/token", map[string]string{"device_code": code.DeviceCode}, http.StatusOK, &token)
	if token.AccessToken == "" {
		t.Fatal("empty CLI token")
	}
	cli := &http.Client{Transport: bearerTransport{token: token.AccessToken, base: http.DefaultTransport}}
	var cliWorkspaces []api.Workspace
	getJSON(t, cli, ts.URL+"/workspaces", http.StatusOK, &cliWorkspaces)
	if len(cliWorkspaces) != 2 {
		t.Fatalf("CLI workspaces=%+v", cliWorkspaces)
	}

	// Recreate the application to prove repository-backed CLI credentials survive a process restart.
	ts.Close()
	app2, err := application.New(cfg)
	if err != nil {
		t.Fatal(err)
	}
	ts2 := httptest.NewServer(app2.Handler())
	defer ts2.Close()
	getJSON(t, cli, ts2.URL+"/workspaces", http.StatusOK, &cliWorkspaces)
	if len(cliWorkspaces) != 2 {
		t.Fatalf("persisted CLI workspaces=%+v", cliWorkspaces)
	}

	memberJar2, _ := cookiejar.New(nil)
	memberBrowser2 := &http.Client{Jar: memberJar2}
	postJSON(
		t,
		memberBrowser2,
		ts2.URL+"/auth/dev",
		map[string]string{"email": "member@example.com", "name": "Member"},
		http.StatusOK,
		nil,
	)
	var persistedMemberWorkspaces []api.Workspace
	getJSON(
		t,
		memberBrowser2,
		ts2.URL+"/workspaces",
		http.StatusOK,
		&persistedMemberWorkspaces,
	)
	if len(persistedMemberWorkspaces) != 2 {
		t.Fatalf("persisted member workspaces=%+v", persistedMemberWorkspaces)
	}
}

type bearerTransport struct {
	token string
	base  http.RoundTripper
}

func (t bearerTransport) RoundTrip(r *http.Request) (*http.Response, error) {
	clone := r.Clone(r.Context())
	clone.Header = r.Header.Clone()
	clone.Header.Set("Authorization", "Bearer "+t.token)
	return t.base.RoundTrip(clone)
}

func postJSON(t *testing.T, client *http.Client, url string, body any, status int, out any) {
	t.Helper()
	data, _ := json.Marshal(body)
	req, err := http.NewRequest(http.MethodPost, url, bytes.NewReader(data))
	if err != nil {
		t.Fatal(err)
	}
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Origin", "http://frontend.test")
	doJSON(t, client, req, status, out)
}
func patchJSON(t *testing.T, client *http.Client, url string, body any, status int, out any) {
	t.Helper()
	data, _ := json.Marshal(body)
	req, err := http.NewRequest(http.MethodPatch, url, bytes.NewReader(data))
	if err != nil {
		t.Fatal(err)
	}
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Origin", "http://frontend.test")
	doJSON(t, client, req, status, out)
}

func requestStatus(t *testing.T, client *http.Client, method, url string, status int) {
	t.Helper()
	req, err := http.NewRequest(method, url, nil)
	if err != nil {
		t.Fatal(err)
	}
	doJSON(t, client, req, status, nil)
}

func getJSON(t *testing.T, client *http.Client, url string, status int, out any) {
	t.Helper()
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		t.Fatal(err)
	}
	doJSON(t, client, req, status, out)
}
func doJSON(t *testing.T, client *http.Client, req *http.Request, status int, out any) {
	t.Helper()
	resp, err := client.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	defer resp.Body.Close()
	data, err := io.ReadAll(resp.Body)
	if err != nil {
		t.Fatal(err)
	}
	if resp.StatusCode != status {
		t.Fatalf("%s %s status=%d want=%d body=%s", req.Method, req.URL, resp.StatusCode, status, data)
	}
	if out != nil && len(data) > 0 {
		if err := json.Unmarshal(data, out); err != nil {
			t.Fatalf("decode %s: %v", data, err)
		}
	}
}
