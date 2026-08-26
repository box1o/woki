package file

import (
	"context"
	"errors"
	"os"
	"path/filepath"
	"testing"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/domain/workspace"
)

func TestStorePersistsDomainState(t *testing.T) {
	path := filepath.Join(t.TempDir(), "state", "woki.json")
	store, err := Open(path)
	if err != nil {
		t.Fatal(err)
	}
	users, workspaces := store.Users(), store.Workspaces()
	u, _ := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	if err := users.Create(context.Background(), u); err != nil {
		t.Fatal(err)
	}
	w, _ := workspace.New("personal", u.ID)
	m, _ := workspace.NewMember(u.ID, w.ID, u.Email, u.Name, workspace.RoleOwner)
	if err := workspaces.CreateWithOwner(context.Background(), w, m); err != nil {
		t.Fatal(err)
	}
	info, err := os.Stat(path)
	if err != nil {
		t.Fatal(err)
	}
	if info.Mode().Perm() != 0o600 {
		t.Fatalf("state mode=%o", info.Mode().Perm())
	}
	reopened, err := Open(path)
	if err != nil {
		t.Fatal(err)
	}
	got, err := reopened.Users().FindByEmail(context.Background(), u.Email)
	if err != nil {
		t.Fatal(err)
	}
	if got.ID != u.ID {
		t.Fatalf("got user %s, want %s", got.ID, u.ID)
	}
	list, err := reopened.Workspaces().ListForUser(context.Background(), u.ID)
	if err != nil {
		t.Fatal(err)
	}
	if len(list) != 1 || list[0].ID != w.ID {
		t.Fatalf("unexpected workspaces: %+v", list)
	}
}

func TestOpenRejectsInvalidSnapshot(t *testing.T) {
	path := filepath.Join(t.TempDir(), "state.json")
	if err := os.WriteFile(path, []byte(`{"users":[{"id":"bad"}]}`), 0o600); err != nil {
		t.Fatal(err)
	}
	if _, err := Open(path); err == nil {
		t.Fatal("Open unexpectedly accepted invalid state")
	}
}

func TestMutationRollsBackWhenPersistenceFailsBeforeCommit(t *testing.T) {
	ctx := context.Background()
	root := t.TempDir()
	blockingPath := filepath.Join(root, "not-a-directory")
	if err := os.WriteFile(blockingPath, []byte("x"), 0o600); err != nil {
		t.Fatal(err)
	}
	store, err := Open(filepath.Join(blockingPath, "woki.json"))
	if err == nil {
		t.Fatal("Open unexpectedly succeeded with a non-directory parent")
	}

	// Open a valid store first, then make its parent unwritable as a directory
	// by replacing the directory before the first persistence attempt.
	path := filepath.Join(root, "state", "woki.json")
	store, err = Open(path)
	if err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Dir(path), []byte("block"), 0o600); err != nil {
		t.Fatal(err)
	}

	u, err := user.New(
		"owner@example.com",
		"Owner",
		"",
		user.ProviderDev,
		"owner@example.com",
	)
	if err != nil {
		t.Fatal(err)
	}
	if err := store.Users().Create(ctx, u); err == nil {
		t.Fatal("Create unexpectedly succeeded")
	}
	if _, err := store.Users().FindByID(ctx, u.ID); !errors.Is(err, user.ErrNotFound) {
		t.Fatalf("rolled-back user lookup error=%v; want ErrNotFound", err)
	}
}

func TestOpenRejectsOversizedStateFile(t *testing.T) {
	path := filepath.Join(t.TempDir(), "woki.json")
	file, err := os.Create(path)
	if err != nil {
		t.Fatal(err)
	}
	if err := file.Truncate(maxStateFileSize + 1); err != nil {
		_ = file.Close()
		t.Fatal(err)
	}
	if err := file.Close(); err != nil {
		t.Fatal(err)
	}
	if _, err := Open(path); !errors.Is(err, ErrStateTooLarge) {
		t.Fatalf("Open()=%v; want ErrStateTooLarge", err)
	}
}
