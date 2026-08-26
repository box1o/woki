package provider

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/config"
)

func TestGitHubExchangeUsesVerifiedEmail(t *testing.T) {
	mux := http.NewServeMux()
	mux.HandleFunc("POST /token", func(w http.ResponseWriter, r *http.Request) {
		if got := r.Header.Get("Accept"); got != "application/json" {
			t.Fatalf("Accept=%q", got)
		}
		w.Header().Set("Content-Type", "application/json")
		_, _ = w.Write([]byte(`{"access_token":"token"}`))
	})
	mux.HandleFunc("GET /user", func(w http.ResponseWriter, r *http.Request) {
		if got := r.Header.Get("Authorization"); got != "Bearer token" {
			t.Fatalf("Authorization=%q", got)
		}
		_, _ = w.Write([]byte(`{"id":12345,"login":"box1o","name":"Box","avatar_url":"https://example/avatar"}`))
	})
	mux.HandleFunc("GET /user/emails", func(w http.ResponseWriter, _ *http.Request) {
		_, _ = w.Write([]byte(`[{"email":"box@example.com","primary":true,"verified":true}]`))
	})
	server := httptest.NewServer(mux)
	defer server.Close()

	github := NewGitHub(config.GitHubConfig{ClientID: "id", ClientSecret: "secret"})
	github.tokenURL = server.URL + "/token"
	github.apiURL = server.URL

	profile, err := github.Exchange(context.Background(), "code")
	if err != nil {
		t.Fatal(err)
	}
	if profile.Email != "box@example.com" || profile.Name != "Box" || profile.Provider != user.ProviderGitHub || profile.ProviderID != "12345" {
		t.Fatalf("unexpected profile: %+v", profile)
	}
}

func TestGitHubExchangeRejectsTokenHTTPFailure(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		http.Error(w, "nope", http.StatusBadGateway)
	}))
	defer server.Close()
	github := NewGitHub(config.GitHubConfig{ClientID: "id", ClientSecret: "secret"})
	github.tokenURL = server.URL
	if _, err := github.Exchange(context.Background(), "code"); err == nil {
		t.Fatal("Exchange unexpectedly succeeded")
	}
}
