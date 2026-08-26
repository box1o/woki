package workspace

import (
	"context"
	"errors"
	"sync"
	"testing"

	"github.com/box1o/woki/internal/domain/user"
	domain "github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/internal/infrastructure/memory"
)

func TestEnsurePersonalIsIdempotentUnderConcurrency(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	users := memory.UserRepository{Store: store}
	ws := memory.WorkspaceRepository{Store: store}
	owner, _ := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	if err := users.Create(ctx, owner); err != nil {
		t.Fatal(err)
	}
	svc := New(ws, users)
	var wg sync.WaitGroup
	errs := make(chan error, 16)
	for range 16 {
		wg.Add(1)
		go func() { defer wg.Done(); _, err := svc.EnsurePersonal(ctx, owner); errs <- err }()
	}
	wg.Wait()
	close(errs)
	for err := range errs {
		if err != nil {
			t.Fatal(err)
		}
	}
	list, err := svc.List(ctx, owner.ID)
	if err != nil {
		t.Fatal(err)
	}
	if len(list) != 1 || list[0].Name != "personal" {
		t.Fatalf("unexpected workspaces: %+v", list)
	}
}
func TestMemberAuthorization(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	users := memory.UserRepository{Store: store}
	ws := memory.WorkspaceRepository{Store: store}
	svc := New(ws, users)
	owner, _ := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	member, _ := user.New("member@example.com", "Member", "", user.ProviderDev, "member@example.com")
	_ = users.Create(ctx, owner)
	_ = users.Create(ctx, member)
	w, err := svc.Create(ctx, owner.ID, "team")
	if err != nil {
		t.Fatal(err)
	}
	m, err := svc.AddMember(ctx, owner.ID, w.ID, member.Email, domain.RoleMember)
	if err != nil {
		t.Fatal(err)
	}
	if _, err := svc.AddMember(ctx, member.ID, w.ID, owner.Email, domain.RoleMember); !errors.Is(err, domain.ErrForbidden) {
		t.Fatalf("got %v", err)
	}
	if err := svc.RemoveMember(ctx, owner.ID, w.ID, m.ID); err != nil {
		t.Fatal(err)
	}
}

func TestOwnerCannotBeRemovedOrDemoted(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	users := memory.UserRepository{Store: store}
	workspaces := memory.WorkspaceRepository{Store: store}
	svc := New(workspaces, users)
	owner, err := user.New("owner2@example.com", "Owner", "", user.ProviderDev, "owner2@example.com")
	if err != nil {
		t.Fatal(err)
	}
	if err := users.Create(ctx, owner); err != nil {
		t.Fatal(err)
	}
	ws, err := svc.Create(ctx, owner.ID, "protected")
	if err != nil {
		t.Fatal(err)
	}
	members, err := svc.ListMembers(ctx, owner.ID, ws.ID)
	if err != nil || len(members) != 1 {
		t.Fatalf("members=%v err=%v", members, err)
	}
	ownerMember := members[0]
	if err := svc.RemoveMember(ctx, owner.ID, ws.ID, ownerMember.ID); !errors.Is(err, domain.ErrOwnerRemoval) {
		t.Fatalf("RemoveMember(owner)=%v; want ErrOwnerRemoval", err)
	}
	if _, err := svc.UpdateMemberRole(ctx, owner.ID, ws.ID, ownerMember.ID, domain.RoleMember); !errors.Is(err, domain.ErrOwnerRemoval) {
		t.Fatalf("UpdateMemberRole(owner)=%v; want ErrOwnerRemoval", err)
	}
}

func TestSearchMemberCandidatesFiltersExistingMembers(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	users := memory.UserRepository{Store: store}
	workspaces := memory.WorkspaceRepository{Store: store}
	svc := New(workspaces, users)

	owner, _ := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	alice, _ := user.New("alice@example.com", "Alice Smith", "", user.ProviderDev, "alice@example.com")
	alicia, _ := user.New("alicia@example.com", "Alicia Jones", "", user.ProviderDev, "alicia@example.com")
	for _, value := range []*user.User{owner, alice, alicia} {
		if err := users.Create(ctx, value); err != nil {
			t.Fatal(err)
		}
	}
	ws, err := svc.Create(ctx, owner.ID, "team")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := svc.AddMember(ctx, owner.ID, ws.ID, alice.Email, domain.RoleMember); err != nil {
		t.Fatal(err)
	}

	results, err := svc.SearchMemberCandidates(ctx, owner.ID, ws.ID, "ali", 8)
	if err != nil {
		t.Fatal(err)
	}
	if len(results) != 1 || results[0].ID != alicia.ID {
		t.Fatalf("unexpected candidates: %#v", results)
	}
}

func TestSearchMemberCandidatesRequiresManager(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	users := memory.UserRepository{Store: store}
	workspaces := memory.WorkspaceRepository{Store: store}
	svc := New(workspaces, users)

	owner, _ := user.New("owner3@example.com", "Owner", "", user.ProviderDev, "owner3@example.com")
	member, _ := user.New("member3@example.com", "Member", "", user.ProviderDev, "member3@example.com")
	candidate, _ := user.New("candidate@example.com", "Candidate", "", user.ProviderDev, "candidate@example.com")
	for _, value := range []*user.User{owner, member, candidate} {
		if err := users.Create(ctx, value); err != nil {
			t.Fatal(err)
		}
	}
	ws, err := svc.Create(ctx, owner.ID, "team-search")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := svc.AddMember(ctx, owner.ID, ws.ID, member.Email, domain.RoleMember); err != nil {
		t.Fatal(err)
	}
	if _, err := svc.SearchMemberCandidates(ctx, member.ID, ws.ID, "can", 8); !errors.Is(err, domain.ErrForbidden) {
		t.Fatalf("SearchMemberCandidates()=%v; want forbidden", err)
	}
}
