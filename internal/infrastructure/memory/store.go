// Package memory provides concurrency-safe in-process repository adapters.
package memory

import (
	"fmt"
	"sort"
	"strings"
	"sync"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/pkg/id"
)

type Store struct {
	mu                sync.RWMutex
	users             map[id.ID]*user.User
	usersByEmail      map[string]id.ID
	usersByProvider   map[string]id.ID
	workspaces        map[id.ID]*workspace.Workspace
	members           map[id.ID]*workspace.Member
	credentials       map[id.ID]*domaincli.Credential
	credentialsByHash map[string]id.ID
}

func New() *Store {
	return &Store{
		users:             make(map[id.ID]*user.User),
		usersByEmail:      make(map[string]id.ID),
		usersByProvider:   make(map[string]id.ID),
		workspaces:        make(map[id.ID]*workspace.Workspace),
		members:           make(map[id.ID]*workspace.Member),
		credentials:       make(map[id.ID]*domaincli.Credential),
		credentialsByHash: make(map[string]id.ID),
	}
}

const SnapshotVersion = 1

type Snapshot struct {
	Version     int                    `json:"version"`
	Users       []user.User            `json:"users"`
	Workspaces  []workspace.Workspace  `json:"workspaces"`
	Members     []workspace.Member     `json:"members"`
	Credentials []domaincli.Credential `json:"credentials"`
}

func (s *Store) Snapshot() Snapshot {
	s.mu.RLock()
	defer s.mu.RUnlock()
	out := Snapshot{Version: SnapshotVersion}
	for _, v := range s.users {
		out.Users = append(out.Users, *cloneUser(v))
	}
	for _, v := range s.workspaces {
		out.Workspaces = append(out.Workspaces, *cloneWorkspace(v))
	}
	for _, v := range s.members {
		out.Members = append(out.Members, *cloneMember(v))
	}
	for _, v := range s.credentials {
		out.Credentials = append(out.Credentials, *cloneCredential(v))
	}
	sort.Slice(out.Users, func(i, j int) bool { return out.Users[i].ID < out.Users[j].ID })
	sort.Slice(out.Workspaces, func(i, j int) bool { return out.Workspaces[i].ID < out.Workspaces[j].ID })
	sort.Slice(out.Members, func(i, j int) bool { return out.Members[i].ID < out.Members[j].ID })
	sort.Slice(out.Credentials, func(i, j int) bool { return out.Credentials[i].ID < out.Credentials[j].ID })
	return out
}

func (s *Store) Restore(snapshot Snapshot) error {
	if snapshot.Version != SnapshotVersion {
		return ErrSnapshotVersion.WithDetail(fmt.Sprint(snapshot.Version))
	}

	users := make(map[id.ID]*user.User, len(snapshot.Users))
	usersByEmail := make(map[string]id.ID, len(snapshot.Users))
	usersByProvider := make(map[string]id.ID, len(snapshot.Users))
	workspaces := make(map[id.ID]*workspace.Workspace, len(snapshot.Workspaces))
	members := make(map[id.ID]*workspace.Member, len(snapshot.Members))
	credentials := make(map[id.ID]*domaincli.Credential, len(snapshot.Credentials))
	credentialsByHash := make(map[string]id.ID, len(snapshot.Credentials))

	for i := range snapshot.Users {
		u := cloneUser(&snapshot.Users[i])
		if err := u.Validate(); err != nil {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("user %s", u.ID)).WithErr(err)
		}
		emailKey := normalizeEmail(u.Email)
		providerKey := providerIdentityKey(u.Provider, u.ProviderID)
		if _, ok := users[u.ID]; ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("duplicate user ID %s", u.ID))
		}
		if _, ok := usersByEmail[emailKey]; ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("duplicate user email %s", emailKey))
		}
		if _, ok := usersByProvider[providerKey]; ok {
			return ErrSnapshotInvalid.WithDetail("duplicate provider identity")
		}
		users[u.ID] = u
		usersByEmail[emailKey] = u.ID
		usersByProvider[providerKey] = u.ID
	}
	for i := range snapshot.Workspaces {
		w := cloneWorkspace(&snapshot.Workspaces[i])
		if err := w.Validate(); err != nil {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("workspace %s", w.ID)).WithErr(err)
		}
		if _, ok := users[w.OwnerID]; !ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("workspace %s owner is missing", w.ID))
		}
		if _, ok := workspaces[w.ID]; ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("duplicate workspace ID %s", w.ID))
		}
		workspaces[w.ID] = w
	}
	seenNames := map[string]struct{}{}
	for _, w := range workspaces {
		key := workspaceNameKey(w.OwnerID, w.Name)
		if _, ok := seenNames[key]; ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("duplicate workspace name %q for owner %s", w.Name, w.OwnerID))
		}
		seenNames[key] = struct{}{}
	}
	seenMemberships := make(map[string]struct{}, len(snapshot.Members))
	for i := range snapshot.Members {
		m := cloneMember(&snapshot.Members[i])
		if err := m.Validate(); err != nil {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("member %s", m.ID)).WithErr(err)
		}
		if _, ok := users[m.UserID]; !ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("member %s user is missing", m.ID))
		}
		w, ok := workspaces[m.WorkspaceID]
		if !ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("member %s workspace is missing", m.ID))
		}
		if _, ok := members[m.ID]; ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("duplicate member ID %s", m.ID))
		}
		if m.Role == workspace.RoleOwner && m.UserID != w.OwnerID {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("member %s has an invalid owner role", m.ID))
		}
		membershipKey := m.WorkspaceID.String() + "\x00" + m.UserID.String()
		if _, ok := seenMemberships[membershipKey]; ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("duplicate membership for user %s", m.UserID))
		}
		seenMemberships[membershipKey] = struct{}{}
		members[m.ID] = m
	}
	for _, w := range workspaces {
		var hasOwner bool
		for _, m := range members {
			if m.WorkspaceID == w.ID && m.UserID == w.OwnerID && m.Role == workspace.RoleOwner {
				hasOwner = true
				break
			}
		}
		if !hasOwner {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("workspace %s owner membership is missing", w.ID))
		}
	}
	for i := range snapshot.Credentials {
		c := cloneCredential(&snapshot.Credentials[i])
		if err := c.Validate(); err != nil {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("credential %s", c.ID)).WithErr(err)
		}
		if _, ok := users[c.OwnerID]; !ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("credential %s owner is missing", c.ID))
		}
		if _, ok := credentials[c.ID]; ok {
			return ErrSnapshotInvalid.WithDetail(fmt.Sprintf("duplicate credential ID %s", c.ID))
		}
		if _, ok := credentialsByHash[c.TokenHash]; ok {
			return ErrSnapshotInvalid.WithDetail("duplicate credential token hash")
		}
		credentials[c.ID], credentialsByHash[c.TokenHash] = c, c.ID
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.users, s.usersByEmail, s.usersByProvider = users, usersByEmail, usersByProvider
	s.workspaces, s.members = workspaces, members
	s.credentials, s.credentialsByHash = credentials, credentialsByHash
	return nil
}

func normalizeEmail(v string) string { return strings.ToLower(strings.TrimSpace(v)) }
func providerIdentityKey(provider user.Provider, providerID string) string {
	return string(provider) + "\x00" + strings.TrimSpace(providerID)
}
func workspaceNameKey(owner id.ID, name string) string {
	return owner.String() + "\x00" + strings.ToLower(strings.TrimSpace(name))
}
func cloneUser(v *user.User) *user.User {
	if v == nil {
		return nil
	}
	x := *v
	return &x
}
func cloneWorkspace(v *workspace.Workspace) *workspace.Workspace {
	if v == nil {
		return nil
	}
	x := *v
	return &x
}
func cloneMember(v *workspace.Member) *workspace.Member {
	if v == nil {
		return nil
	}
	x := *v
	return &x
}
func cloneCredential(v *domaincli.Credential) *domaincli.Credential {
	if v == nil {
		return nil
	}
	x := *v
	return &x
}
