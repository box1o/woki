package workspace

import (
	"context"
	"errors"
	"fmt"
	"strings"

	client "github.com/box1o/woki/cli/internal/api"
	"github.com/box1o/woki/cli/internal/credentials"
	"github.com/box1o/woki/pkg/api"
)

type Store interface {
	Load(context.Context) (credentials.Credential, error)
}

type Service struct {
	client  *client.Client
	store   Store
	current CurrentStore
}

func New(client *client.Client, store Store, current ...CurrentStore) *Service {
	var selection CurrentStore
	if len(current) > 0 {
		selection = current[0]
	}
	return &Service{client: client, store: store, current: selection}
}

func (s *Service) authenticated(ctx context.Context) (*client.Client, error) {
	credential, err := s.store.Load(ctx)
	if err != nil {
		return nil, err
	}
	return s.client.WithToken(credential.AccessToken), nil
}

func (s *Service) List(ctx context.Context) ([]api.Workspace, error) {
	c, err := s.authenticated(ctx)
	if err != nil {
		return nil, err
	}
	return c.ListWorkspaces(ctx)
}

func (s *Service) Create(ctx context.Context, name string) (api.Workspace, error) {
	c, err := s.authenticated(ctx)
	if err != nil {
		return api.Workspace{}, err
	}
	return c.CreateWorkspace(ctx, name)
}

func (s *Service) SetCurrent(ctx context.Context, ref string) (api.Workspace, error) {
	workspace, err := s.Resolve(ctx, ref)
	if err != nil {
		return api.Workspace{}, err
	}
	if s.current == nil {
		return api.Workspace{}, ErrSelectionWrite.WithDetail("workspace selection store is not configured")
	}
	if err := s.current.Save(ctx, s.client.BaseURL(), Current{ID: workspace.ID, Name: workspace.Name}); err != nil {
		return api.Workspace{}, err
	}
	return workspace, nil
}

func (s *Service) Selection(ctx context.Context) (Current, error) {
	if s.current == nil {
		return Current{}, ErrCurrentNotSet
	}
	return s.current.Load(ctx, s.client.BaseURL())
}

func (s *Service) Current(ctx context.Context) (api.Workspace, error) {
	if s.current == nil {
		return api.Workspace{}, ErrCurrentNotSet
	}
	selection, err := s.current.Load(ctx, s.client.BaseURL())
	if err != nil {
		return api.Workspace{}, err
	}
	values, err := s.List(ctx)
	if err != nil {
		return api.Workspace{}, err
	}
	for _, workspace := range values {
		if workspace.ID == selection.ID {
			if workspace.Name != selection.Name {
				_ = s.current.Save(ctx, s.client.BaseURL(), Current{ID: workspace.ID, Name: workspace.Name})
			}
			return workspace, nil
		}
	}
	// IDs are authoritative, but resolving the previous name is useful after
	// a backend migration that preserved names while regenerating identifiers.
	for _, workspace := range values {
		if strings.EqualFold(workspace.Name, selection.Name) {
			_ = s.current.Save(ctx, s.client.BaseURL(), Current{ID: workspace.ID, Name: workspace.Name})
			return workspace, nil
		}
	}
	return api.Workspace{}, ErrCurrentNotSet.WithDetail("the previously selected workspace no longer exists; run `woki workspace use`")
}

func (s *Service) Resolve(ctx context.Context, ref string) (api.Workspace, error) {
	ref = strings.TrimSpace(ref)
	if ref == "" {
		return s.Current(ctx)
	}
	values, err := s.List(ctx)
	if err != nil {
		return api.Workspace{}, err
	}
	for _, workspace := range values {
		if workspace.ID == ref {
			return workspace, nil
		}
	}
	var matches []api.Workspace
	for _, workspace := range values {
		if strings.EqualFold(workspace.Name, ref) {
			matches = append(matches, workspace)
		}
	}
	switch len(matches) {
	case 1:
		return matches[0], nil
	case 0:
		return api.Workspace{}, ErrWorkspaceNotFound.WithDetail(ref)
	default:
		return api.Workspace{}, ErrWorkspaceAmbiguous.WithDetail(ref)
	}
}

func (s *Service) Delete(ctx context.Context, ref string) (api.Workspace, error) {
	workspace, err := s.Resolve(ctx, ref)
	if err != nil {
		return api.Workspace{}, err
	}
	c, err := s.authenticated(ctx)
	if err != nil {
		return api.Workspace{}, err
	}
	if err := c.DeleteWorkspace(ctx, workspace.ID); err != nil {
		return api.Workspace{}, err
	}
	if s.current != nil {
		selection, loadErr := s.current.Load(ctx, s.client.BaseURL())
		if loadErr == nil && selection.ID == workspace.ID {
			if err := s.current.Delete(ctx, s.client.BaseURL()); err != nil {
				return workspace, err
			}
		}
	}
	return workspace, nil
}

func (s *Service) Members(ctx context.Context, ref string) ([]api.Member, error) {
	workspace, err := s.Resolve(ctx, ref)
	if err != nil {
		return nil, err
	}
	c, err := s.authenticated(ctx)
	if err != nil {
		return nil, err
	}
	return c.ListMembers(ctx, workspace.ID)
}

func (s *Service) Candidates(ctx context.Context, ref, query string, limit int) ([]api.User, error) {
	workspace, err := s.Resolve(ctx, ref)
	if err != nil {
		return nil, err
	}
	c, err := s.authenticated(ctx)
	if err != nil {
		return nil, err
	}
	return c.MemberCandidates(ctx, workspace.ID, query, limit)
}

func (s *Service) Add(ctx context.Context, ref, email, role string) (api.Member, error) {
	workspace, err := s.Resolve(ctx, ref)
	if err != nil {
		return api.Member{}, err
	}
	c, err := s.authenticated(ctx)
	if err != nil {
		return api.Member{}, err
	}
	return c.AddMember(ctx, workspace.ID, email, role)
}

func (s *Service) Remove(ctx context.Context, ref, memberRef string) (api.Member, error) {
	workspace, err := s.Resolve(ctx, ref)
	if err != nil {
		return api.Member{}, err
	}
	member, err := s.resolveMember(ctx, workspace.ID, memberRef)
	if err != nil {
		return api.Member{}, err
	}
	c, err := s.authenticated(ctx)
	if err != nil {
		return api.Member{}, err
	}
	if err := c.RemoveMember(ctx, workspace.ID, member.ID); err != nil {
		return api.Member{}, err
	}
	return member, nil
}

func (s *Service) Role(ctx context.Context, ref, memberRef, role string) (api.Member, error) {
	workspace, err := s.Resolve(ctx, ref)
	if err != nil {
		return api.Member{}, err
	}
	member, err := s.resolveMember(ctx, workspace.ID, memberRef)
	if err != nil {
		return api.Member{}, err
	}
	c, err := s.authenticated(ctx)
	if err != nil {
		return api.Member{}, err
	}
	return c.UpdateMemberRole(ctx, workspace.ID, member.ID, role)
}

func (s *Service) resolveMember(ctx context.Context, workspaceID, ref string) (api.Member, error) {
	ref = strings.TrimSpace(ref)
	if ref == "" {
		return api.Member{}, ErrMemberNotFound
	}
	c, err := s.authenticated(ctx)
	if err != nil {
		return api.Member{}, err
	}
	members, err := c.ListMembers(ctx, workspaceID)
	if err != nil {
		return api.Member{}, err
	}
	for _, member := range members {
		if member.ID == ref || strings.EqualFold(member.Email, ref) {
			return member, nil
		}
	}
	return api.Member{}, ErrMemberNotFound.WithDetail(fmt.Sprintf("%q", ref))
}

func (s *Service) ClearCurrent(ctx context.Context) error {
	if s.current == nil {
		return nil
	}
	err := s.current.Delete(ctx, s.client.BaseURL())
	if errors.Is(err, ErrCurrentNotSet) {
		return nil
	}
	return err
}
