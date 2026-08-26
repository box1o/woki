package workspace

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	client "github.com/box1o/woki/cli/internal/api"
	"github.com/box1o/woki/cli/internal/credentials"
	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/httpx"
)

type staticStore struct{ credential credentials.Credential }

func (s staticStore) Load(context.Context) (credentials.Credential, error) { return s.credential, nil }

func TestListUsesStoredBearerCredential(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if got := r.Header.Get("Authorization"); got != "Bearer secret" {
			t.Fatalf("Authorization=%q", got)
		}
		httpx.WriteJSON(w, http.StatusOK, []api.Workspace{{ID: "one", Name: "personal"}})
	}))
	defer server.Close()

	apiClient, err := client.New(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	svc := New(apiClient, staticStore{credential: credentials.Credential{
		AccessToken: "secret",
		ExpiresAt:   time.Now().Add(time.Hour),
	}})

	values, err := svc.List(context.Background())
	if err != nil {
		t.Fatal(err)
	}
	if len(values) != 1 || values[0].Name != "personal" {
		t.Fatalf("unexpected workspaces: %#v", values)
	}
}

type memoryCurrentStore struct{ current Current }

func (s *memoryCurrentStore) Load(context.Context, string) (Current, error) {
	if s.current.ID == "" {
		return Current{}, ErrCurrentNotSet
	}
	return s.current, nil
}
func (s *memoryCurrentStore) Save(_ context.Context, _ string, current Current) error {
	s.current = current
	return nil
}
func (s *memoryCurrentStore) Delete(context.Context, string) error {
	s.current = Current{}
	return nil
}

func TestSetCurrentResolvesWorkspaceByName(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/workspaces" {
			t.Fatalf("path=%q", r.URL.Path)
		}
		httpx.WriteJSON(w, http.StatusOK, []api.Workspace{
			{ID: "one", Name: "personal"},
			{ID: "two", Name: "Project Alpha"},
		})
	}))
	defer server.Close()

	apiClient, err := client.New(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	current := &memoryCurrentStore{}
	svc := New(apiClient, staticStore{credential: credentials.Credential{
		AccessToken: "secret",
		ExpiresAt:   time.Now().Add(time.Hour),
	}}, current)

	selected, err := svc.SetCurrent(context.Background(), "project alpha")
	if err != nil {
		t.Fatal(err)
	}
	if selected.ID != "two" || current.current.ID != "two" {
		t.Fatalf("selected=%#v current=%#v", selected, current.current)
	}
}

func TestMembersUsesCurrentWorkspaceWhenReferenceIsEmpty(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/workspaces":
			httpx.WriteJSON(w, http.StatusOK, []api.Workspace{{ID: "one", Name: "personal"}})
		case "/workspaces/one/members":
			httpx.WriteJSON(w, http.StatusOK, []api.Member{{ID: "m1", Email: "member@example.com", Role: "member"}})
		default:
			t.Fatalf("unexpected path=%q", r.URL.Path)
		}
	}))
	defer server.Close()

	apiClient, err := client.New(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	current := &memoryCurrentStore{current: Current{ID: "one", Name: "personal"}}
	svc := New(apiClient, staticStore{credential: credentials.Credential{
		AccessToken: "secret",
		ExpiresAt:   time.Now().Add(time.Hour),
	}}, current)

	members, err := svc.Members(context.Background(), "")
	if err != nil {
		t.Fatal(err)
	}
	if len(members) != 1 || members[0].Email != "member@example.com" {
		t.Fatalf("members=%#v", members)
	}
}
