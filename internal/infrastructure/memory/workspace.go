package memory

import (
	"context"
	"sort"
	"strings"
	"time"

	"github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/pkg/id"
)

func (s *Store) CreateWorkspaceWithOwner(_ context.Context, value *workspace.Workspace, owner *workspace.Member) error {
	if value == nil || owner == nil {
		return workspace.ErrNotFound
	}
	if err := value.Validate(); err != nil {
		return err
	}
	if err := owner.Validate(); err != nil {
		return err
	}
	if owner.WorkspaceID != value.ID || owner.UserID != value.OwnerID || owner.Role != workspace.RoleOwner {
		return workspace.ErrOwnerRequired
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.workspaces[value.ID]; ok {
		return workspace.ErrAlreadyExists
	}
	for _, existing := range s.workspaces {
		if existing.OwnerID == value.OwnerID && strings.EqualFold(strings.TrimSpace(existing.Name), strings.TrimSpace(value.Name)) {
			return workspace.ErrAlreadyExists
		}
	}
	if _, ok := s.users[value.OwnerID]; !ok {
		return workspace.ErrOwnerRequired
	}
	s.workspaces[value.ID] = cloneWorkspace(value)
	s.members[owner.ID] = cloneMember(owner)
	return nil
}

func (s *Store) FindWorkspaceByOwnerAndName(_ context.Context, ownerID id.ID, name string) (*workspace.Workspace, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	for _, w := range s.workspaces {
		if w.OwnerID == ownerID && strings.EqualFold(strings.TrimSpace(w.Name), strings.TrimSpace(name)) {
			return cloneWorkspace(w), nil
		}
	}
	return nil, workspace.ErrNotFound
}

func (s *Store) ListWorkspacesForUser(_ context.Context, userID id.ID) ([]*workspace.Workspace, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	allowed := map[id.ID]struct{}{}
	for _, m := range s.members {
		if m.UserID == userID {
			allowed[m.WorkspaceID] = struct{}{}
		}
	}
	out := make([]*workspace.Workspace, 0, len(allowed))
	for wid := range allowed {
		if w := s.workspaces[wid]; w != nil {
			out = append(out, cloneWorkspace(w))
		}
	}
	sort.Slice(out, func(i, j int) bool {
		if strings.EqualFold(out[i].Name, out[j].Name) {
			return out[i].ID < out[j].ID
		}
		return strings.ToLower(out[i].Name) < strings.ToLower(out[j].Name)
	})
	return out, nil
}

func (s *Store) GetWorkspace(_ context.Context, workspaceID id.ID) (*workspace.Workspace, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	w, ok := s.workspaces[workspaceID]
	if !ok {
		return nil, workspace.ErrNotFound
	}
	return cloneWorkspace(w), nil
}

func (s *Store) DeleteWorkspace(_ context.Context, workspaceID id.ID) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.workspaces[workspaceID]; !ok {
		return workspace.ErrNotFound
	}
	delete(s.workspaces, workspaceID)
	for mid, m := range s.members {
		if m.WorkspaceID == workspaceID {
			delete(s.members, mid)
		}
	}
	return nil
}

func (s *Store) FindWorkspaceMember(_ context.Context, workspaceID, userID id.ID) (*workspace.Member, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	for _, m := range s.members {
		if m.WorkspaceID == workspaceID && m.UserID == userID {
			return cloneMember(m), nil
		}
	}
	return nil, workspace.ErrMemberNotFound
}
func (s *Store) GetWorkspaceMember(_ context.Context, workspaceID, memberID id.ID) (*workspace.Member, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	m, ok := s.members[memberID]
	if !ok || m.WorkspaceID != workspaceID {
		return nil, workspace.ErrMemberNotFound
	}
	return cloneMember(m), nil
}
func (s *Store) ListWorkspaceMembers(_ context.Context, workspaceID id.ID) ([]*workspace.Member, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	if _, ok := s.workspaces[workspaceID]; !ok {
		return nil, workspace.ErrNotFound
	}
	out := []*workspace.Member{}
	for _, m := range s.members {
		if m.WorkspaceID == workspaceID {
			out = append(out, cloneMember(m))
		}
	}
	sort.Slice(out, func(i, j int) bool {
		if out[i].Role != out[j].Role {
			return out[i].Role < out[j].Role
		}
		return strings.ToLower(out[i].Email) < strings.ToLower(out[j].Email)
	})
	return out, nil
}

func (s *Store) AddWorkspaceMember(_ context.Context, value *workspace.Member) error {
	if value == nil {
		return workspace.ErrMemberNotFound
	}
	if err := value.Validate(); err != nil {
		return err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	w, ok := s.workspaces[value.WorkspaceID]
	if !ok {
		return workspace.ErrNotFound
	}
	if _, ok := s.users[value.UserID]; !ok {
		return workspace.ErrMemberUserRequired
	}
	if value.Role == workspace.RoleOwner && value.UserID != w.OwnerID {
		return workspace.ErrInvalidRole
	}
	for _, m := range s.members {
		if m.WorkspaceID == value.WorkspaceID && m.UserID == value.UserID {
			return workspace.ErrMemberAlreadyExists
		}
	}
	s.members[value.ID] = cloneMember(value)
	return nil
}
func (s *Store) RemoveWorkspaceMember(_ context.Context, workspaceID, memberID id.ID) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	m, ok := s.members[memberID]
	if !ok || m.WorkspaceID != workspaceID {
		return workspace.ErrMemberNotFound
	}
	if m.Role == workspace.RoleOwner {
		return workspace.ErrOwnerRemoval
	}
	delete(s.members, memberID)
	return nil
}
func (s *Store) UpdateWorkspaceMemberRole(_ context.Context, workspaceID, memberID id.ID, role workspace.Role) (*workspace.Member, error) {
	if role == workspace.RoleOwner {
		return nil, workspace.ErrOwnerRemoval
	}
	if _, err := workspace.ParseRole(string(role)); err != nil {
		return nil, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	m, ok := s.members[memberID]
	if !ok || m.WorkspaceID != workspaceID {
		return nil, workspace.ErrMemberNotFound
	}
	if m.Role == workspace.RoleOwner {
		return nil, workspace.ErrOwnerRemoval
	}
	m = cloneMember(m)
	m.Role = role
	m.UpdatedAt = time.Now().UTC()
	s.members[memberID] = m
	return cloneMember(m), nil
}
