package provider

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/config"
)

func TestGoogleExchangeRequiresVerifiedEmail(t *testing.T) {
	mux := http.NewServeMux()
	mux.HandleFunc("POST /token", func(w http.ResponseWriter, r *http.Request) {
		if got := r.Header.Get("Content-Type"); got != "application/x-www-form-urlencoded" {
			t.Fatalf("Content-Type=%q", got)
		}
		_, _ = w.Write([]byte(`{"access_token":"token"}`))
	})
	mux.HandleFunc("GET /userinfo", func(w http.ResponseWriter, r *http.Request) {
		if got := r.Header.Get("Authorization"); got != "Bearer token" {
			t.Fatalf("Authorization=%q", got)
		}
		_, _ = w.Write([]byte(`{"sub":"12345","email":"user@example.com","email_verified":true,"name":"User"}`))
	})
	server := httptest.NewServer(mux)
	defer server.Close()

	google := NewGoogle(config.GoogleConfig{
		ClientID:     "client-id",
		ClientSecret: "client-secret",
		CallbackURL:  "http://localhost:3000/auth/google/callback",
	})
	google.tokenURL = server.URL + "/token"
	google.userInfoURL = server.URL + "/userinfo"

	profile, err := google.Exchange(context.Background(), "code")
	if err != nil {
		t.Fatal(err)
	}
	if profile.Email != "user@example.com" || profile.Provider != user.ProviderGoogle || profile.ProviderID != "12345" {
		t.Fatalf("unexpected profile: %+v", profile)
	}
}

func TestGoogleExchangeRejectsUnverifiedEmail(t *testing.T) {
	mux := http.NewServeMux()
	mux.HandleFunc("POST /token", func(w http.ResponseWriter, _ *http.Request) {
		_, _ = w.Write([]byte(`{"access_token":"token"}`))
	})
	mux.HandleFunc("GET /userinfo", func(w http.ResponseWriter, _ *http.Request) {
		_, _ = w.Write([]byte(`{"sub":"12345","email":"user@example.com","email_verified":false}`))
	})
	server := httptest.NewServer(mux)
	defer server.Close()

	google := NewGoogle(config.GoogleConfig{
		ClientID:     "client-id",
		ClientSecret: "client-secret",
		CallbackURL:  "http://localhost:3000/auth/google/callback",
	})
	google.tokenURL = server.URL + "/token"
	google.userInfoURL = server.URL + "/userinfo"

	if _, err := google.Exchange(context.Background(), "code"); err == nil {
		t.Fatal("Exchange unexpectedly succeeded")
	}
}
