// Package workspace implements workspace use cases and authorization rules.
package workspace

import (
	"context"
	stderrors "errors"
	"strings"

	domainevents "github.com/box1o/woki/internal/domain/events"
	"github.com/box1o/woki/internal/domain/user"
	domain "github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/pkg/id"
	"github.com/box1o/woki/pkg/log"
)

type Service struct {
	workspaces domain.Repository
	users      user.Repository
	events     domainevents.Bus
}

type MemberCandidate struct {
	ID        id.ID
	Email     string
	Name      string
	AvatarURL string
}

func New(workspaces domain.Repository, users user.Repository) *Service {
	return NewWithEvents(workspaces, users, nil)
}

func NewWithEvents(workspaces domain.Repository, users user.Repository, events domainevents.Bus) *Service {
	return &Service{workspaces: workspaces, users: users, events: events}
}

func (s *Service) EnsurePersonal(ctx context.Context, owner *user.User) (*domain.Workspace, error) {
	if owner == nil {
		return nil, user.ErrNotFound
	}
	found, err := s.workspaces.FindByOwnerAndName(ctx, owner.ID, "personal")
	if err == nil {
		return found, nil
	}
	if !stderrors.Is(err, domain.ErrNotFound) {
		return nil, wrapRead(err)
	}

	created, err := s.createOwned(ctx, "personal", owner)
	if stderrors.Is(err, domain.ErrAlreadyExists) {
		found, findErr := s.workspaces.FindByOwnerAndName(ctx, owner.ID, "personal")
		if findErr != nil {
			return nil, wrapRead(findErr)
		}
		return found, nil
	}
	return created, err
}

func (s *Service) Create(ctx context.Context, actorID id.ID, name string) (*domain.Workspace, error) {
	owner, err := s.users.FindByID(ctx, actorID)
	if err != nil {
		if stderrors.Is(err, user.ErrNotFound) {
			return nil, err
		}
		return nil, wrapRead(err)
	}
	return s.createOwned(ctx, name, owner)
}

func (s *Service) createOwned(ctx context.Context, name string, owner *user.User) (*domain.Workspace, error) {
	workspace, err := domain.New(name, owner.ID)
	if err != nil {
		return nil, err
	}
	ownerMember, err := domain.NewMember(
		owner.ID,
		workspace.ID,
		owner.Email,
		owner.Name,
		domain.RoleOwner,
	)
	if err != nil {
		return nil, err
	}
	ownerMember.AvatarURL = owner.AvatarURL
	if err := s.workspaces.CreateWithOwner(ctx, workspace, ownerMember); err != nil {
		if stderrors.Is(err, domain.ErrAlreadyExists) {
			return nil, err
		}
		return nil, wrapWrite(err)
	}
	if s.events != nil {
		if err := s.events.Publish(ctx, domain.NewWorkspaceCreated(workspace, owner.Email)); err != nil {
			log.Warn("publish workspace-created event for %s: %v", workspace.ID, err)
		}
	}
	return workspace, nil
}

func (s *Service) List(ctx context.Context, actorID id.ID) ([]*domain.Workspace, error) {
	if actorID.IsZero() {
		return nil, domain.ErrForbidden
	}
	values, err := s.workspaces.ListForUser(ctx, actorID)
	if err != nil {
		return nil, wrapRead(err)
	}
	return values, nil
}

func (s *Service) Delete(ctx context.Context, actorID, workspaceID id.ID) error {
	member, err := s.workspaces.FindMember(ctx, workspaceID, actorID)
	if err != nil {
		return authorizationError(err)
	}
	if member.Role != domain.RoleOwner {
		return domain.ErrForbidden
	}
	if err := s.workspaces.Delete(ctx, workspaceID); err != nil {
		if expectedDomainError(err) {
			return err
		}
		return wrapWrite(err)
	}
	return nil
}

func (s *Service) ListMembers(ctx context.Context, actorID, workspaceID id.ID) ([]*domain.Member, error) {
	if _, err := s.workspaces.FindMember(ctx, workspaceID, actorID); err != nil {
		return nil, authorizationError(err)
	}
	members, err := s.workspaces.ListMembers(ctx, workspaceID)
	if err != nil {
		if expectedDomainError(err) {
			return nil, err
		}
		return nil, wrapRead(err)
	}
	return members, nil
}

func (s *Service) SearchMemberCandidates(
	ctx context.Context,
	actorID, workspaceID id.ID,
	query string,
	limit int,
) ([]MemberCandidate, error) {
	if err := s.requireManager(ctx, actorID, workspaceID); err != nil {
		return nil, err
	}

	query = strings.TrimSpace(query)
	if len(query) < 2 {
		return nil, ErrSearchQuery.WithDetail("query must contain at least 2 characters")
	}
	if len(query) > 100 {
		return nil, ErrSearchQuery.WithDetail("query must not exceed 100 characters")
	}
	if limit <= 0 {
		limit = 8
	}
	if limit > 20 {
		limit = 20
	}

	members, err := s.workspaces.ListMembers(ctx, workspaceID)
	if err != nil {
		if expectedDomainError(err) {
			return nil, err
		}
		return nil, wrapRead(err)
	}
	existing := make(map[id.ID]struct{}, len(members))
	for _, member := range members {
		existing[member.UserID] = struct{}{}
	}

	// Search enough rows to compensate for users who are already members. The
	// hard cap keeps even very large workspaces from turning autocomplete into
	// an unbounded directory query.
	scanLimit := min(limit+len(existing), 100)
	users, err := s.users.Search(ctx, query, scanLimit)
	if err != nil {
		return nil, wrapRead(err)
	}

	result := make([]MemberCandidate, 0, min(limit, len(users)))
	for _, candidate := range users {
		if _, ok := existing[candidate.ID]; ok {
			continue
		}
		result = append(result, MemberCandidate{
			ID:        candidate.ID,
			Email:     candidate.Email,
			Name:      candidate.Name,
			AvatarURL: candidate.AvatarURL,
		})
		if len(result) == limit {
			break
		}
	}
	return result, nil
}

func (s *Service) AddMember(
	ctx context.Context,
	actorID, workspaceID id.ID,
	email string,
	role domain.Role,
) (*domain.Member, error) {
	if err := s.requireManager(ctx, actorID, workspaceID); err != nil {
		return nil, err
	}
	if role == domain.RoleOwner {
		return nil, domain.ErrInvalidRole
	}

	target, err := s.users.FindByEmail(ctx, email)
	if err != nil {
		if stderrors.Is(err, user.ErrNotFound) {
			return nil, err
		}
		return nil, wrapRead(err)
	}
	if _, err := s.workspaces.FindMember(ctx, workspaceID, target.ID); err == nil {
		return nil, domain.ErrMemberAlreadyExists
	} else if !stderrors.Is(err, domain.ErrMemberNotFound) {
		return nil, wrapRead(err)
	}

	member, err := domain.NewMember(target.ID, workspaceID, target.Email, target.Name, role)
	if err != nil {
		return nil, err
	}
	member.AvatarURL = target.AvatarURL
	if err := s.workspaces.AddMember(ctx, member); err != nil {
		if expectedDomainError(err) {
			return nil, err
		}
		return nil, wrapWrite(err)
	}
	if s.events != nil {
		if current, err := s.workspaces.Get(ctx, workspaceID); err == nil {
			if err := s.events.Publish(ctx, domain.NewMemberAdded(current, member)); err != nil {
				log.Warn("publish workspace-member-added event for %s: %v", member.ID, err)
			}
		}
	}
	return member, nil
}

func (s *Service) RemoveMember(ctx context.Context, actorID, workspaceID, memberID id.ID) error {
	if err := s.requireManager(ctx, actorID, workspaceID); err != nil {
		return err
	}
	target, err := s.workspaces.GetMember(ctx, workspaceID, memberID)
	if err != nil {
		if expectedDomainError(err) {
			return err
		}
		return wrapRead(err)
	}
	if target.Role == domain.RoleOwner {
		return domain.ErrOwnerRemoval
	}
	current, _ := s.workspaces.Get(ctx, workspaceID)
	if err := s.workspaces.RemoveMember(ctx, workspaceID, memberID); err != nil {
		if expectedDomainError(err) {
			return err
		}
		return wrapWrite(err)
	}
	if s.events != nil && current != nil {
		if err := s.events.Publish(ctx, domain.NewMemberRemoved(current, target)); err != nil {
			log.Warn("publish workspace-member-removed event for %s: %v", target.ID, err)
		}
	}
	return nil
}

func (s *Service) UpdateMemberRole(
	ctx context.Context,
	actorID, workspaceID, memberID id.ID,
	role domain.Role,
) (*domain.Member, error) {
	if err := s.requireManager(ctx, actorID, workspaceID); err != nil {
		return nil, err
	}
	if role == domain.RoleOwner {
		return nil, domain.ErrOwnerRemoval
	}
	member, err := s.workspaces.UpdateMemberRole(ctx, workspaceID, memberID, role)
	if err != nil {
		if expectedDomainError(err) {
			return nil, err
		}
		return nil, wrapWrite(err)
	}
	return member, nil
}

func (s *Service) requireManager(ctx context.Context, actorID, workspaceID id.ID) error {
	member, err := s.workspaces.FindMember(ctx, workspaceID, actorID)
	if err != nil {
		return authorizationError(err)
	}
	if !member.Role.CanManageMembers() {
		return domain.ErrForbidden
	}
	return nil
}

func authorizationError(err error) error {
	if stderrors.Is(err, domain.ErrMemberNotFound) || stderrors.Is(err, domain.ErrNotFound) {
		// Deliberately hide workspace existence from unauthorized callers.
		return domain.ErrForbidden
	}
	return wrapRead(err)
}

func expectedDomainError(err error) bool {
	return stderrors.Is(err, domain.ErrNotFound) ||
		stderrors.Is(err, domain.ErrAlreadyExists) ||
		stderrors.Is(err, domain.ErrMemberNotFound) ||
		stderrors.Is(err, domain.ErrMemberAlreadyExists) ||
		stderrors.Is(err, domain.ErrOwnerRemoval) ||
		stderrors.Is(err, domain.ErrInvalidRole) ||
		stderrors.Is(err, domain.ErrForbidden)
}

func wrapRead(err error) error {
	if err == nil {
		return nil
	}
	return ErrRepositoryRead.WithErr(err)
}

func wrapWrite(err error) error {
	if err == nil {
		return nil
	}
	return ErrRepositoryWrite.WithErr(err)
}
