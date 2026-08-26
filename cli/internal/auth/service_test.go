package auth

import (
	"context"
	"errors"
	"net/http"
	"net/http/httptest"
	"sync/atomic"
	"testing"
	"time"

	client "github.com/box1o/woki/cli/internal/api"
	"github.com/box1o/woki/cli/internal/credentials"
	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/httpx"
)

type testStore struct {
	credential credentials.Credential
	saveErr    error
	deleted    atomic.Int32
}

func (s *testStore) Save(context.Context, credentials.Credential) error { return s.saveErr }
func (s *testStore) Load(context.Context) (credentials.Credential, error) {
	if !s.credential.Valid(time.Now()) {
		return credentials.Credential{}, credentials.ErrNotFound
	}
	return s.credential, nil
}
func (s *testStore) Delete(context.Context) error {
	s.deleted.Add(1)
	return nil
}

func TestLoginRevokesTokenWhenLocalSaveFails(t *testing.T) {
	var revoked atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/auth/device/code":
			httpx.WriteJSON(w, http.StatusOK, api.DeviceCodeResponse{
				DeviceCode:      "device",
				UserCode:        "ABCDEFGH",
				VerificationURI: "https://example/device",
				ExpiresIn:       60,
				Interval:        1,
			})
		case "/auth/device/token":
			httpx.WriteJSON(w, http.StatusOK, api.DeviceTokenResponse{
				AccessToken: "token",
				TokenType:   "Bearer",
				ExpiresAt:   time.Now().Add(time.Hour),
				Owner:       api.User{Email: "user@example.com"},
			})
		case "/auth/cli/logout":
			if r.Header.Get("Authorization") != "Bearer token" {
				t.Fatalf("Authorization=%q", r.Header.Get("Authorization"))
			}
			revoked.Add(1)
			w.WriteHeader(http.StatusNoContent)
		default:
			http.NotFound(w, r)
		}
	}))
	defer server.Close()

	apiClient, err := client.New(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	store := &testStore{saveErr: errors.New("disk full")}
	svc := New(apiClient, store, nil)
	svc.sleep = func(context.Context, time.Duration) error { return nil }

	if _, err := svc.Login(context.Background(), "Woki CLI", nil); err == nil {
		t.Fatal("Login unexpectedly succeeded")
	}
	if revoked.Load() != 1 {
		t.Fatalf("revocations=%d; want 1", revoked.Load())
	}
}

func TestStatusDeletesRejectedLocalCredential(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/auth/cli/status" {
			http.NotFound(w, r)
			return
		}
		httpx.WriteError(w, http.StatusUnauthorized, "unauthorized", "expired")
	}))
	defer server.Close()

	apiClient, err := client.New(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	store := &testStore{credential: credentials.Credential{
		AccessToken: "token",
		ExpiresAt:   time.Now().Add(time.Hour),
	}}
	svc := New(apiClient, store, nil)

	if _, _, err := svc.Status(context.Background()); err == nil {
		t.Fatal("Status unexpectedly succeeded")
	}
	if store.deleted.Load() != 1 {
		t.Fatalf("deletions=%d; want 1", store.deleted.Load())
	}
}

func TestLogoutTreatsRejectedRemoteCredentialAsAlreadyRevoked(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/auth/cli/logout" {
			http.NotFound(w, r)
			return
		}
		httpx.WriteError(w, http.StatusUnauthorized, "CLI_CREDENTIAL_EXPIRED", "credential expired")
	}))
	defer server.Close()

	apiClient, err := client.New(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	store := &testStore{credential: credentials.Credential{
		AccessToken: "expired-remotely",
		ExpiresAt:   time.Now().Add(time.Hour),
	}}
	svc := New(apiClient, store, nil)

	if err := svc.Logout(context.Background()); err != nil {
		t.Fatalf("Logout()=%v; want nil", err)
	}
	if store.deleted.Load() != 1 {
		t.Fatalf("deletions=%d; want 1", store.deleted.Load())
	}
}

func TestLoginPropagatesPresentationFailure(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/auth/device/code" {
			http.NotFound(w, r)
			return
		}
		httpx.WriteJSON(w, http.StatusOK, api.DeviceCodeResponse{
			DeviceCode:      "device",
			UserCode:        "ABCDEFGH",
			VerificationURI: "https://example/device",
			ExpiresIn:       60,
			Interval:        1,
		})
	}))
	defer server.Close()

	apiClient, err := client.New(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	svc := New(apiClient, &testStore{}, nil)
	want := errors.New("broken output")
	_, err = svc.Login(context.Background(), "Woki CLI", func(api.DeviceCodeResponse) error { return want })
	if !errors.Is(err, want) {
		t.Fatalf("Login()=%v; want presentation error", err)
	}
}
