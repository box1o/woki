package memory

import (
	"context"
	"errors"
	"testing"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/domain/workspace"
)

func TestUserUpdateMaintainsEmailIndex(t *testing.T) {
	ctx := context.Background()
	store := New()
	repo := UserRepository{Store: store}
	u, _ := user.New("old@example.com", "User", "", user.ProviderDev, "old@example.com")
	if err := repo.Create(ctx, u); err != nil {
		t.Fatal(err)
	}
	if err := u.UpdateProfile("new@example.com", "User", "", user.ProviderDev, "old@example.com"); err != nil {
		t.Fatal(err)
	}
	if err := repo.Update(ctx, u); err != nil {
		t.Fatal(err)
	}
	if _, err := repo.FindByEmail(ctx, "old@example.com"); !errors.Is(err, user.ErrNotFound) {
		t.Fatalf("old email lookup=%v", err)
	}
	got, err := repo.FindByEmail(ctx, "new@example.com")
	if err != nil {
		t.Fatal(err)
	}
	if got.ID != u.ID {
		t.Fatalf("got %s want %s", got.ID, u.ID)
	}
}

func TestWorkspaceNameUniquePerOwner(t *testing.T) {
	ctx := context.Background()
	store := New()
	users := UserRepository{Store: store}
	workspaces := WorkspaceRepository{Store: store}
	u, _ := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	_ = users.Create(ctx, u)
	first, _ := workspace.New("Team", u.ID)
	owner, _ := workspace.NewMember(u.ID, first.ID, u.Email, u.Name, workspace.RoleOwner)
	if err := workspaces.CreateWithOwner(ctx, first, owner); err != nil {
		t.Fatal(err)
	}
	second, _ := workspace.New(" team ", u.ID)
	owner2, _ := workspace.NewMember(u.ID, second.ID, u.Email, u.Name, workspace.RoleOwner)
	if err := workspaces.CreateWithOwner(ctx, second, owner2); !errors.Is(err, workspace.ErrAlreadyExists) {
		t.Fatalf("duplicate workspace err=%v", err)
	}
}

func TestSnapshotRestoreRejectsBrokenOwnerMembership(t *testing.T) {
	ctx := context.Background()
	store := New()
	users := UserRepository{Store: store}
	workspaces := WorkspaceRepository{Store: store}
	u, _ := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	_ = users.Create(ctx, u)
	w, _ := workspace.New("Team", u.ID)
	m, _ := workspace.NewMember(u.ID, w.ID, u.Email, u.Name, workspace.RoleOwner)
	_ = workspaces.CreateWithOwner(ctx, w, m)
	snapshot := store.Snapshot()
	snapshot.Members = nil
	if err := New().Restore(snapshot); err == nil {
		t.Fatal("Restore accepted workspace without owner membership")
	}
}
