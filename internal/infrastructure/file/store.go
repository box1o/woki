// Package file persists the in-memory repositories as one atomic JSON snapshot.
package file

import (
	"context"
	"encoding/json"
	"errors"
	"io/fs"
	"os"
	"path/filepath"
	"runtime"
	"sync"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/internal/infrastructure/memory"
	"github.com/box1o/woki/pkg/id"
)

const maxStateFileSize = 16 << 20 // 16 MiB

type Store struct {
	mu   sync.RWMutex
	path string
	mem  *memory.Store
}

func Open(path string) (*Store, error) {
	path = filepath.Clean(path)
	if path == "." || path == "" {
		return nil, ErrPathRequired
	}
	s := &Store{path: path, mem: memory.New()}
	info, err := os.Stat(path)
	if errors.Is(err, fs.ErrNotExist) {
		return s, nil
	}
	if err != nil {
		return nil, ErrReadFailed.WithErr(err)
	}
	if info.Size() == 0 {
		return nil, ErrStateEmpty.WithDetail(path)
	}
	if info.Size() > maxStateFileSize {
		return nil, ErrStateTooLarge.WithDetail(path)
	}
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, ErrReadFailed.WithErr(err)
	}
	var snapshot memory.Snapshot
	if err := json.Unmarshal(data, &snapshot); err != nil {
		return nil, ErrDecodeFailed.WithErr(err)
	}
	if err := s.mem.Restore(snapshot); err != nil {
		return nil, ErrStateInvalid.WithErr(err)
	}
	return s, nil
}

func (s *Store) mutate(fn func() error) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	before := s.mem.Snapshot()
	if err := fn(); err != nil {
		return err
	}
	committed, err := s.persistLocked()
	if err == nil {
		return nil
	}
	if committed {
		return ErrDurability.WithErr(err)
	}
	if restoreErr := s.mem.Restore(before); restoreErr != nil {
		return ErrRollbackFailed.WithDetail(err.Error()).WithErr(restoreErr)
	}
	return err
}

// persistLocked writes the current snapshot. committed reports whether the atomic
// rename has already made the new state visible at the configured path.
func (s *Store) persistLocked() (committed bool, err error) {
	snapshot := s.mem.Snapshot()
	data, err := json.MarshalIndent(snapshot, "", "  ")
	if err != nil {
		return false, ErrPersistFailed.WithDetail("encode state").WithErr(err)
	}
	data = append(data, '\n')

	dir := filepath.Dir(s.path)
	if err := os.MkdirAll(dir, 0o700); err != nil {
		return false, ErrPersistFailed.WithDetail("create storage directory").WithErr(err)
	}
	if runtime.GOOS != "windows" {
		if err := os.Chmod(dir, 0o700); err != nil {
			return false, ErrPersistFailed.WithDetail("secure storage directory").WithErr(err)
		}
	}

	tmp, err := os.CreateTemp(dir, ".woki-state-*")
	if err != nil {
		return false, ErrPersistFailed.WithDetail("create temporary state file").WithErr(err)
	}
	tmpName := tmp.Name()
	cleanup := func() {
		_ = tmp.Close()
		_ = os.Remove(tmpName)
	}
	if runtime.GOOS != "windows" {
		if err := tmp.Chmod(0o600); err != nil {
			cleanup()
			return false, ErrPersistFailed.WithDetail("secure temporary state file").WithErr(err)
		}
	}
	if _, err := tmp.Write(data); err != nil {
		cleanup()
		return false, ErrPersistFailed.WithDetail("write state").WithErr(err)
	}
	if err := tmp.Sync(); err != nil {
		cleanup()
		return false, ErrPersistFailed.WithDetail("sync state file").WithErr(err)
	}
	if err := tmp.Close(); err != nil {
		_ = os.Remove(tmpName)
		return false, ErrPersistFailed.WithDetail("close state file").WithErr(err)
	}
	if err := os.Rename(tmpName, s.path); err != nil {
		_ = os.Remove(tmpName)
		return false, ErrPersistFailed.WithDetail("replace state file").WithErr(err)
	}

	// Rename is the commit point: from here onward the new state is visible.
	// Directory fsync is required for durable renames on Unix. Windows does not
	// provide portable directory syncing through os.File.Sync.
	if runtime.GOOS != "windows" {
		if err := syncDirectory(dir); err != nil {
			return true, ErrDurability.WithDetail("sync storage directory").WithErr(err)
		}
	}
	return true, nil
}

func syncDirectory(dir string) error {
	dirFD, err := os.Open(dir)
	if err != nil {
		return err
	}
	if err := dirFD.Sync(); err != nil {
		_ = dirFD.Close()
		return err
	}
	return dirFD.Close()
}

type UserRepository struct{ store *Store }
type WorkspaceRepository struct{ store *Store }
type CredentialRepository struct{ store *Store }

func (s *Store) Users() UserRepository             { return UserRepository{s} }
func (s *Store) Workspaces() WorkspaceRepository   { return WorkspaceRepository{s} }
func (s *Store) Credentials() CredentialRepository { return CredentialRepository{s} }

func (r UserRepository) Create(c context.Context, v *user.User) error {
	return r.store.mutate(func() error { return r.store.mem.CreateUser(c, v) })
}
func (r UserRepository) Update(c context.Context, v *user.User) error {
	return r.store.mutate(func() error { return r.store.mem.UpdateUser(c, v) })
}
func (r UserRepository) FindByID(c context.Context, v id.ID) (*user.User, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.FindUserByID(c, v)
}
func (r UserRepository) FindByEmail(c context.Context, v string) (*user.User, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.FindUserByEmail(c, v)
}
func (r UserRepository) FindByProvider(
	c context.Context,
	provider user.Provider,
	providerID string,
) (*user.User, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.FindUserByProvider(c, provider, providerID)
}
func (r UserRepository) Search(c context.Context, query string, limit int) ([]*user.User, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.SearchUsers(c, query, limit)
}

func (r WorkspaceRepository) CreateWithOwner(c context.Context, w *workspace.Workspace, m *workspace.Member) error {
	return r.store.mutate(func() error { return r.store.mem.CreateWorkspaceWithOwner(c, w, m) })
}
func (r WorkspaceRepository) FindByOwnerAndName(c context.Context, o id.ID, n string) (*workspace.Workspace, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.FindWorkspaceByOwnerAndName(c, o, n)
}
func (r WorkspaceRepository) ListForUser(c context.Context, u id.ID) ([]*workspace.Workspace, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.ListWorkspacesForUser(c, u)
}
func (r WorkspaceRepository) Get(c context.Context, w id.ID) (*workspace.Workspace, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.GetWorkspace(c, w)
}
func (r WorkspaceRepository) Delete(c context.Context, w id.ID) error {
	return r.store.mutate(func() error { return r.store.mem.DeleteWorkspace(c, w) })
}
func (r WorkspaceRepository) FindMember(c context.Context, w, u id.ID) (*workspace.Member, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.FindWorkspaceMember(c, w, u)
}
func (r WorkspaceRepository) GetMember(c context.Context, w, m id.ID) (*workspace.Member, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.GetWorkspaceMember(c, w, m)
}
func (r WorkspaceRepository) ListMembers(c context.Context, w id.ID) ([]*workspace.Member, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.ListWorkspaceMembers(c, w)
}
func (r WorkspaceRepository) AddMember(c context.Context, m *workspace.Member) error {
	return r.store.mutate(func() error { return r.store.mem.AddWorkspaceMember(c, m) })
}
func (r WorkspaceRepository) RemoveMember(c context.Context, w, m id.ID) error {
	return r.store.mutate(func() error { return r.store.mem.RemoveWorkspaceMember(c, w, m) })
}
func (r WorkspaceRepository) UpdateMemberRole(c context.Context, w, m id.ID, role workspace.Role) (*workspace.Member, error) {
	var out *workspace.Member
	err := r.store.mutate(func() error {
		var err error
		out, err = r.store.mem.UpdateWorkspaceMemberRole(c, w, m, role)
		return err
	})
	return out, err
}

func (r CredentialRepository) Create(c context.Context, v *domaincli.Credential) error {
	return r.store.mutate(func() error { return r.store.mem.CreateCredential(c, v) })
}
func (r CredentialRepository) FindByID(c context.Context, v id.ID) (*domaincli.Credential, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.FindCredentialByID(c, v)
}
func (r CredentialRepository) FindByTokenHash(c context.Context, v string) (*domaincli.Credential, error) {
	r.store.mu.RLock()
	defer r.store.mu.RUnlock()
	return r.store.mem.FindCredentialByTokenHash(c, v)
}
func (r CredentialRepository) Delete(c context.Context, v id.ID) error {
	return r.store.mutate(func() error { return r.store.mem.DeleteCredential(c, v) })
}
