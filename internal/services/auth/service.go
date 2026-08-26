// Package auth implements browser authentication use cases.
package auth

import (
	"context"
	"errors"
	"strings"

	domainevents "github.com/box1o/woki/internal/domain/events"
	"github.com/box1o/woki/internal/domain/user"
	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/id"
	"github.com/box1o/woki/pkg/log"
)

type Profile struct {
	Email      string
	Name       string
	AvatarURL  string
	Provider   user.Provider
	ProviderID string
}

type SessionManager interface {
	Create(context.Context, id.ID) (string, error)
	Get(context.Context, string) (id.ID, error)
	Delete(context.Context, string) error
}

type Service struct {
	users          user.Repository
	sessions       SessionManager
	ensurePersonal func(context.Context, *user.User) error
	events         domainevents.Bus
}

func New(
	users user.Repository,
	sessions SessionManager,
	ensurePersonal func(context.Context, *user.User) error,
) *Service {
	return NewWithEvents(users, sessions, ensurePersonal, nil)
}

func NewWithEvents(
	users user.Repository,
	sessions SessionManager,
	ensurePersonal func(context.Context, *user.User) error,
	events domainevents.Bus,
) *Service {
	return &Service{users: users, sessions: sessions, ensurePersonal: ensurePersonal, events: events}
}

func (s *Service) Login(ctx context.Context, profile Profile) (*user.User, string, error) {
	profile.ProviderID = strings.TrimSpace(profile.ProviderID)
	if profile.Provider == user.ProviderDev && profile.ProviderID == "" {
		profile.ProviderID = strings.ToLower(strings.TrimSpace(profile.Email))
	}

	usr, err := s.users.FindByProvider(ctx, profile.Provider, profile.ProviderID)
	isNew := false
	switch {
	case err == nil:
		if err := usr.UpdateProfile(
			profile.Email,
			profile.Name,
			profile.AvatarURL,
			profile.Provider,
			profile.ProviderID,
		); err != nil {
			return nil, "", err
		}
		if err := s.users.Update(ctx, usr); err != nil {
			if errors.Is(err, user.ErrAlreadyExists) {
				return nil, "", ErrIdentityConflict.WithErr(err)
			}
			return nil, "", ErrUserUpdateFailed.WithErr(err)
		}
	case errors.Is(err, user.ErrNotFound):
		// Email remains unique across identities. Do not silently link a new
		// provider identity to an existing account based on email alone.
		if _, emailErr := s.users.FindByEmail(ctx, profile.Email); emailErr == nil {
			return nil, "", ErrIdentityConflict
		} else if !errors.Is(emailErr, user.ErrNotFound) {
			return nil, "", ErrUserLookupFailed.WithErr(emailErr)
		}

		isNew = true
		usr, err = user.New(
			profile.Email,
			profile.Name,
			profile.AvatarURL,
			profile.Provider,
			profile.ProviderID,
		)
		if err != nil {
			return nil, "", err
		}
		if err = s.users.Create(ctx, usr); errors.Is(err, user.ErrAlreadyExists) {
			// Another request may have created the same provider identity. Resolve
			// that race first; otherwise the collision is an email owned by a
			// different identity and must never be linked implicitly.
			if existing, findErr := s.users.FindByProvider(ctx, profile.Provider, profile.ProviderID); findErr == nil {
				usr, err = existing, nil
			} else if _, emailErr := s.users.FindByEmail(ctx, profile.Email); emailErr == nil {
				return nil, "", ErrIdentityConflict.WithErr(err)
			} else if !errors.Is(emailErr, user.ErrNotFound) {
				return nil, "", ErrUserLookupFailed.WithErr(emailErr)
			} else {
				err = findErr
			}
		}
		if err != nil {
			return nil, "", ErrUserCreateFailed.WithErr(err)
		}
	default:
		return nil, "", ErrUserLookupFailed.WithErr(err)
	}

	if err := s.ensurePersonal(ctx, usr); err != nil {
		return nil, "", ErrWorkspaceProvision.WithErr(err)
	}
	token, err := s.sessions.Create(ctx, usr.ID)
	if err != nil {
		return nil, "", ErrSessionCreate.WithErr(err)
	}
	if isNew && s.events != nil {
		if err := s.events.Publish(ctx, user.NewAccountCreated(usr)); err != nil {
			log.Warn("publish account-created event for %s: %v", usr.ID, err)
		}
	}
	return usr, token, nil
}

func (s *Service) UserFromSession(ctx context.Context, token string) (*user.User, error) {
	uid, err := s.sessions.Get(ctx, token)
	if err != nil {
		switch {
		case apperrors.IsCode(err, "SESSION_NOT_FOUND"):
			return nil, ErrSessionNotFound.WithErr(err)
		case apperrors.IsCode(err, "SESSION_STORAGE_FAILED"):
			return nil, ErrSessionUnavailable.WithErr(err)
		default:
			return nil, ErrSessionResolve.WithErr(err)
		}
	}
	usr, err := s.users.FindByID(ctx, uid)
	if err != nil {
		switch {
		case errors.Is(err, user.ErrNotFound):
			return nil, ErrSessionNotFound.WithErr(err)
		case errors.Is(err, user.ErrDatabaseOperation):
			return nil, ErrSessionUnavailable.WithErr(err)
		default:
			return nil, ErrSessionResolve.WithErr(err)
		}
	}
	return usr, nil
}

func (s *Service) Logout(ctx context.Context, token string) error {
	return s.sessions.Delete(ctx, token)
}
