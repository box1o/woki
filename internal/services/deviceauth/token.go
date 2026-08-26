package deviceauth

import (
	"context"
	"strings"
	"time"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/id"
	"github.com/box1o/woki/pkg/log"
)

func (s *Service) Exchange(ctx context.Context, deviceCode string) (api.DeviceTokenResponse, error) {
	deviceCode = strings.TrimSpace(deviceCode)
	if deviceCode == "" || len(deviceCode) > 256 {
		return api.DeviceTokenResponse{}, ErrInvalidCode
	}
	deviceHash := hash(deviceCode)

	s.mu.Lock()
	s.cleanupLocked(time.Now())
	state, ok := s.byDevice[deviceHash]
	if !ok {
		s.mu.Unlock()
		return api.DeviceTokenResponse{}, ErrAuthorizationExpired
	}

	switch state.Status {
	case statusPending, statusExchanging:
		s.mu.Unlock()
		return api.DeviceTokenResponse{}, ErrAuthorizationPending
	case statusDenied:
		s.mu.Unlock()
		return api.DeviceTokenResponse{}, ErrAuthorizationDenied
	case statusApproved:
		state.Status = statusExchanging
	default:
		s.mu.Unlock()
		return api.DeviceTokenResponse{}, ErrInvalidCode
	}

	ownerID := state.OwnerID
	clientName := state.ClientName
	expiresAt := state.ExpiresAt
	s.mu.Unlock()

	owner, err := s.users.FindByID(ctx, ownerID)
	if err != nil {
		s.resetExchange(deviceHash)
		return api.DeviceTokenResponse{}, err
	}

	accessToken, err := randomToken(32)
	if err != nil {
		s.resetExchange(deviceHash)
		return api.DeviceTokenResponse{}, ErrTokenGeneration.WithErr(err)
	}
	credential, err := domaincli.NewCredential(
		ownerID,
		clientName,
		hash(accessToken),
		time.Now().UTC().Add(s.credentialTTL),
	)
	if err != nil {
		s.resetExchange(deviceHash)
		return api.DeviceTokenResponse{}, err
	}
	if err := s.credentials.Create(ctx, credential); err != nil {
		s.resetExchange(deviceHash)
		return api.DeviceTokenResponse{}, ErrCredentialCreate.WithErr(err)
	}

	s.mu.Lock()
	current, ok := s.byDevice[deviceHash]
	if !ok || current.Status != statusExchanging || !time.Now().Before(expiresAt) {
		s.mu.Unlock()
		if err := s.credentials.Delete(ctx, credential.ID); err != nil {
			log.Warn("failed to clean up undelivered CLI credential %s: %v", credential.ID, err)
		}
		return api.DeviceTokenResponse{}, ErrAuthorizationExpired
	}
	s.deleteStateLocked(current)
	s.mu.Unlock()

	return api.DeviceTokenResponse{
		AccessToken: accessToken,
		TokenType:   "Bearer",
		ExpiresAt:   credential.ExpiresAt,
		Owner: api.User{
			ID:        owner.ID.String(),
			Email:     owner.Email,
			Name:      owner.Name,
			AvatarURL: owner.AvatarURL,
		},
	}, nil
}

func (s *Service) Authenticate(
	ctx context.Context,
	token string,
) (*user.User, *domaincli.Credential, error) {
	token = strings.TrimSpace(token)
	if token == "" || len(token) > 256 {
		return nil, nil, domaincli.ErrCredentialNotFound
	}

	credential, err := s.credentials.FindByTokenHash(ctx, hash(token))
	if err != nil {
		return nil, nil, err
	}
	if err := credential.Check(time.Now()); err != nil {
		if cleanupErr := s.credentials.Delete(ctx, credential.ID); cleanupErr != nil {
			log.Warn("failed to remove expired CLI credential %s: %v", credential.ID, cleanupErr)
		}
		return nil, nil, err
	}

	owner, err := s.users.FindByID(ctx, credential.OwnerID)
	if err != nil {
		return nil, nil, err
	}
	return owner, credential, nil
}

func (s *Service) Revoke(ctx context.Context, credentialID id.ID) error {
	return s.credentials.Delete(ctx, credentialID)
}
