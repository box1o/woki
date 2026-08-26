package middleware

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
)

type browserStub struct{ usr *user.User }

func (b browserStub) UserFromSession(context.Context, string) (*user.User, error) {
	return b.usr, nil
}

type cliStub struct {
	usr  *user.User
	cred *domaincli.Credential
}

func (c cliStub) Authenticate(context.Context, string) (*user.User, *domaincli.Credential, error) {
	return c.usr, c.cred, nil
}

func TestRequireWebRejectsForeignOriginForMutation(t *testing.T) {
	usr := testUser(t)
	auth := NewAuthenticator(
		browserStub{usr: usr},
		cliStub{},
		"woki_session",
		"https://woki.example",
	)
	handler := auth.RequireWeb(http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		w.WriteHeader(http.StatusNoContent)
	}))
	req := httptest.NewRequest(http.MethodPost, "/workspaces", nil)
	req.Header.Set("Origin", "https://evil.example")
	req.AddCookie(&http.Cookie{Name: "woki_session", Value: "session"})
	w := httptest.NewRecorder()

	handler.ServeHTTP(w, req)

	if w.Code != http.StatusForbidden {
		t.Fatalf("status=%d body=%s", w.Code, w.Body.String())
	}
}

func TestRequireAnyAllowsBearerWithoutBrowserOrigin(t *testing.T) {
	usr := testUser(t)
	cred, err := domaincli.NewCredential(
		usr.ID,
		"Woki CLI",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		time.Now().Add(time.Hour),
	)
	if err != nil {
		t.Fatal(err)
	}
	auth := NewAuthenticator(
		browserStub{},
		cliStub{usr: usr, cred: cred},
		"woki_session",
		"https://woki.example",
	)
	handler := auth.RequireAny(http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		w.WriteHeader(http.StatusNoContent)
	}))
	req := httptest.NewRequest(http.MethodDelete, "/workspaces/id", nil)
	req.Header.Set("Origin", "https://evil.example")
	req.Header.Set("Authorization", "Bearer token")
	w := httptest.NewRecorder()

	handler.ServeHTTP(w, req)

	if w.Code != http.StatusNoContent {
		t.Fatalf("status=%d body=%s", w.Code, w.Body.String())
	}
}

func testUser(t *testing.T) *user.User {
	t.Helper()
	usr, err := user.New(
		"user@example.com",
		"User",
		"",
		user.ProviderDev,
		"user@example.com",
	)
	if err != nil {
		t.Fatal(err)
	}
	return usr
}
