package workspace

import (
	"context"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/infrastructure/memory"
	"github.com/box1o/woki/internal/interfaces/server/middleware"
	service "github.com/box1o/woki/internal/services/workspace"
)

type handlerCLIAuth struct{ usr *user.User }

func (a handlerCLIAuth) Authenticate(context.Context, string) (*user.User, *domaincli.Credential, error) {
	return a.usr, &domaincli.Credential{}, nil
}

type handlerBrowserAuth struct{}

func (handlerBrowserAuth) UserFromSession(context.Context, string) (*user.User, error) {
	return nil, user.ErrNotFound
}

func TestHandlerRejectsInvalidWorkspaceID(t *testing.T) {
	store := memory.New()
	users := memory.UserRepository{Store: store}
	workspaces := memory.WorkspaceRepository{Store: store}
	usr, err := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	if err != nil {
		t.Fatal(err)
	}
	if err := users.Create(context.Background(), usr); err != nil {
		t.Fatal(err)
	}
	auth := middleware.NewAuthenticator(handlerBrowserAuth{}, handlerCLIAuth{usr: usr}, "woki_session", "https://woki.example")
	h := New(service.New(workspaces, users), auth)
	mux := http.NewServeMux()
	h.Register(mux)

	req := httptest.NewRequest(http.MethodDelete, "/workspaces/not-an-id", nil)
	req.Header.Set("Authorization", "Bearer token")
	w := httptest.NewRecorder()
	mux.ServeHTTP(w, req)

	if w.Code != http.StatusBadRequest {
		t.Fatalf("status=%d body=%s", w.Code, w.Body.String())
	}
	if !strings.Contains(w.Body.String(), `"ID_INVALID"`) {
		t.Fatalf("unexpected error response: %s", w.Body.String())
	}
}

func TestMemberCandidatesReturnsOnlyUsersNotInWorkspace(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	users := memory.UserRepository{Store: store}
	workspaces := memory.WorkspaceRepository{Store: store}
	owner, _ := user.New("owner-search@example.com", "Owner", "", user.ProviderDev, "owner-search@example.com")
	alice, _ := user.New("alice-search@example.com", "Alice Search", "", user.ProviderDev, "alice-search@example.com")
	alicia, _ := user.New("alicia-search@example.com", "Alicia Search", "", user.ProviderDev, "alicia-search@example.com")
	for _, usr := range []*user.User{owner, alice, alicia} {
		if err := users.Create(ctx, usr); err != nil {
			t.Fatal(err)
		}
	}

	svc := service.New(workspaces, users)
	ws, err := svc.Create(ctx, owner.ID, "search-team")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := svc.AddMember(ctx, owner.ID, ws.ID, alice.Email, "member"); err != nil {
		t.Fatal(err)
	}

	auth := middleware.NewAuthenticator(handlerBrowserAuth{}, handlerCLIAuth{usr: owner}, "woki_session", "https://woki.example")
	h := New(svc, auth)
	mux := http.NewServeMux()
	h.Register(mux)

	req := httptest.NewRequest(http.MethodGet, "/workspaces/"+ws.ID.String()+"/member-candidates?q=ali", nil)
	req.Header.Set("Authorization", "Bearer token")
	w := httptest.NewRecorder()
	mux.ServeHTTP(w, req)

	if w.Code != http.StatusOK {
		t.Fatalf("status=%d body=%s", w.Code, w.Body.String())
	}
	if strings.Contains(w.Body.String(), alice.Email) {
		t.Fatalf("existing member leaked into candidates: %s", w.Body.String())
	}
	if !strings.Contains(w.Body.String(), alicia.Email) {
		t.Fatalf("candidate missing from response: %s", w.Body.String())
	}
}
