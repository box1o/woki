package auth

import (
	"context"
	"errors"
	"time"

	client "github.com/box1o/woki/cli/internal/api"
	"github.com/box1o/woki/cli/internal/credentials"
	"github.com/box1o/woki/pkg/api"
)

type Browser interface{ Open(string) error }
type Store interface {
	Save(context.Context, credentials.Credential) error
	Load(context.Context) (credentials.Credential, error)
	Delete(context.Context) error
}
type Service struct {
	client  *client.Client
	store   Store
	browser Browser
	sleep   func(context.Context, time.Duration) error
}

func New(client *client.Client, store Store, browser Browser) *Service {
	return &Service{client: client, store: store, browser: browser, sleep: sleepContext}
}
func (s *Service) Login(ctx context.Context, clientName string, onCode func(api.DeviceCodeResponse) error) (credentials.Credential, error) {
	code, err := s.client.CreateDeviceCode(ctx, clientName)
	if err != nil {
		return credentials.Credential{}, err
	}
	if onCode != nil {
		if err := onCode(code); err != nil {
			return credentials.Credential{}, err
		}
	}
	if s.browser != nil {
		// Browser launch is best-effort because the verification URI and code
		// have already been presented for manual completion.
		_ = s.browser.Open(code.VerificationURIComplete)
	}
	interval := time.Duration(code.Interval) * time.Second
	if interval <= 0 {
		interval = 2 * time.Second
	}
	deadline := time.Now().Add(time.Duration(code.ExpiresIn) * time.Second)
	for time.Now().Before(deadline) {
		if err := s.sleep(ctx, interval); err != nil {
			return credentials.Credential{}, err
		}
		token, err := s.client.ExchangeDeviceCode(ctx, code.DeviceCode)
		if err == nil {
			credential := credentials.Credential{
				AccessToken: token.AccessToken,
				TokenType:   token.TokenType,
				ExpiresAt:   token.ExpiresAt,
				Owner:       token.Owner,
			}
			if err := s.store.Save(ctx, credential); err != nil {
				revokeErr := s.client.WithToken(token.AccessToken).CLILogout(ctx)
				if revokeErr != nil {
					return credentials.Credential{}, errors.Join(
						ErrCredentialSave.WithErr(err),
						ErrCredentialRevoke.WithErr(revokeErr),
					)
				}
				return credentials.Credential{}, ErrCredentialSave.WithErr(err)
			}
			return credential, nil
		}
		var apiErr *client.Error
		if errors.As(err, &apiErr) && apiErr.Code == api.ErrorCodeDeviceAuthorizationPending {
			continue
		}
		return credentials.Credential{}, err
	}
	return credentials.Credential{}, ErrAuthorizationExpired
}
func (s *Service) Status(ctx context.Context) (credentials.Credential, map[string]any, error) {
	credential, err := s.store.Load(ctx)
	if err != nil {
		return credentials.Credential{}, nil, err
	}
	status, err := s.client.WithToken(credential.AccessToken).CLIStatus(ctx)
	if err != nil {
		var apiErr *client.Error
		if errors.As(err, &apiErr) && apiErr.Status == 401 {
			if deleteErr := s.store.Delete(ctx); deleteErr != nil {
				return credentials.Credential{}, nil, errors.Join(err, deleteErr)
			}
		}
		return credentials.Credential{}, nil, err
	}
	return credential, status, nil
}
func (s *Service) Logout(ctx context.Context) error {
	credential, err := s.store.Load(ctx)
	if err != nil {
		if errors.Is(err, credentials.ErrNotFound) {
			return nil
		}
		return err
	}
	remoteErr := s.client.WithToken(credential.AccessToken).CLILogout(ctx)
	var apiErr *client.Error
	if errors.As(remoteErr, &apiErr) && (apiErr.Status == 401 || apiErr.Status == 404) {
		remoteErr = nil
	}
	localErr := s.store.Delete(ctx)
	if remoteErr != nil && localErr != nil {
		return errors.Join(remoteErr, localErr)
	}
	if remoteErr != nil {
		return remoteErr
	}
	return localErr
}
func sleepContext(ctx context.Context, d time.Duration) error {
	timer := time.NewTimer(d)
	defer timer.Stop()
	select {
	case <-ctx.Done():
		return ctx.Err()
	case <-timer.C:
		return nil
	}
}
