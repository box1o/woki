package deviceauth

import (
	"context"
	"crypto/rand"
	"encoding/base64"
	stderrors "errors"
	"strings"
	"time"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	infraredis "github.com/box1o/woki/internal/infrastructure/redis"
	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/id"
	"github.com/box1o/woki/pkg/log"
)

type RedisService struct {
	frontendURL   string
	deviceTTL     time.Duration
	credentialTTL time.Duration
	prefix        string
	redis         *infraredis.Client
	credentials   domaincli.Repository
	users         user.Repository
}

func NewRedisService(frontendURL string, deviceTTL, credentialTTL time.Duration, prefix string, redis *infraredis.Client, credentials domaincli.Repository, users user.Repository) *RedisService {
	return &RedisService{frontendURL: strings.TrimRight(frontendURL, "/"), deviceTTL: deviceTTL, credentialTTL: credentialTTL, prefix: strings.Trim(strings.TrimSpace(prefix), ":"), redis: redis, credentials: credentials, users: users}
}
func (s *RedisService) deviceKey(hash string) string { return s.redis.Key(s.prefix, "state", hash) }
func (s *RedisService) userKey(code string) string {
	return s.redis.Key(s.prefix, "user", strings.ToUpper(strings.TrimSpace(code)))
}
func (s *RedisService) lockKey(value string) string { return s.redis.Key(s.prefix, "lock", value) }

func (s *RedisService) Create(ctx context.Context, clientName string) (api.DeviceCodeResponse, error) {
	clientName = strings.TrimSpace(clientName)
	if clientName == "" {
		clientName = "Woki CLI"
	}
	if len(clientName) > 100 {
		return api.DeviceCodeResponse{}, ErrInvalidCode.WithDetail("client name must not exceed 100 characters")
	}
	deviceCode, err := randomToken(32)
	if err != nil {
		return api.DeviceCodeResponse{}, ErrCodeGenerationFailed.WithErr(err)
	}
	deviceHash := hash(deviceCode)
	var userCode string
	for range 32 {
		code, err := randomCode("ABCDEFGHJKLMNPQRSTUVWXYZ23456789", 8)
		if err != nil {
			return api.DeviceCodeResponse{}, ErrCodeGenerationFailed.WithErr(err)
		}
		ok, err := s.redis.SetNX(ctx, s.userKey(code), deviceHash, s.deviceTTL)
		if err != nil {
			return api.DeviceCodeResponse{}, ErrStateStorage.WithErr(err)
		}
		if ok {
			userCode = code
			break
		}
	}
	if userCode == "" {
		return api.DeviceCodeResponse{}, ErrCodeGenerationFailed.WithDetail("unable to reserve user code")
	}
	now := time.Now().UTC()
	state := deviceState{DeviceHash: deviceHash, UserCode: userCode, ClientName: clientName, Status: statusPending, ExpiresAt: now.Add(s.deviceTTL)}
	if err := s.redis.JSONSet(ctx, s.deviceKey(deviceHash), state, s.deviceTTL); err != nil {
		_ = s.redis.Delete(ctx, s.userKey(userCode))
		return api.DeviceCodeResponse{}, ErrStateStorage.WithErr(err)
	}
	uri := s.frontendURL + "/device"
	return api.DeviceCodeResponse{DeviceCode: deviceCode, UserCode: userCode, VerificationURI: uri, VerificationURIComplete: uri + "?code=" + userCode, ExpiresIn: int(s.deviceTTL.Seconds()), Interval: pollInterval}, nil
}
func (s *RedisService) loadByUserCode(ctx context.Context, userCode string) (*deviceState, error) {
	deviceHash, err := s.redis.Get(ctx, s.userKey(userCode))
	if err != nil {
		return nil, ErrAuthorizationExpired
	}
	return s.loadByHash(ctx, deviceHash)
}
func (s *RedisService) loadByHash(ctx context.Context, deviceHash string) (*deviceState, error) {
	var state deviceState
	if err := s.redis.JSONGet(ctx, s.deviceKey(deviceHash), &state); err != nil {
		return nil, ErrAuthorizationExpired
	}
	if !time.Now().Before(state.ExpiresAt) {
		_ = s.deleteState(ctx, &state)
		return nil, ErrAuthorizationExpired
	}
	return &state, nil
}
func (s *RedisService) saveState(ctx context.Context, state *deviceState) error {
	ttl := time.Until(state.ExpiresAt)
	if ttl <= 0 {
		return ErrAuthorizationExpired
	}
	if err := s.redis.JSONSet(ctx, s.deviceKey(state.DeviceHash), state, ttl); err != nil {
		return ErrStateStorage.WithErr(err)
	}
	return nil
}
func (s *RedisService) deleteState(ctx context.Context, state *deviceState) error {
	return s.redis.Delete(ctx, s.deviceKey(state.DeviceHash), s.userKey(state.UserCode))
}
func (s *RedisService) acquire(ctx context.Context, key string) (func(), error) {
	tokenBytes := make([]byte, 16)
	if _, err := rand.Read(tokenBytes); err != nil {
		return nil, ErrStateStorage.WithErr(err)
	}
	token := base64.RawURLEncoding.EncodeToString(tokenBytes)
	lock := s.lockKey(key)
	for i := 0; i < 20; i++ {
		ok, err := s.redis.SetNX(ctx, lock, token, 5*time.Second)
		if err != nil {
			return nil, ErrStateStorage.WithErr(err)
		}
		if ok {
			return func() {
				releaseCtx, cancel := context.WithTimeout(context.Background(), time.Second)
				defer cancel()
				if _, err := s.redis.CompareAndDelete(releaseCtx, lock, token); err != nil {
					log.Warn("failed to release device authorization lock: %v", err)
				}
			}, nil
		}
		select {
		case <-ctx.Done():
			return nil, ctx.Err()
		case <-time.After(25 * time.Millisecond):
		}
	}
	return nil, ErrStateStorage.WithDetail("device state is busy")
}
func (s *RedisService) Inspect(ctx context.Context, userCode string) (api.DeviceRequest, error) {
	state, err := s.loadByUserCode(ctx, userCode)
	if err != nil {
		return api.DeviceRequest{}, err
	}
	if state.Status != statusPending {
		return api.DeviceRequest{}, ErrAlreadyHandled
	}
	return api.DeviceRequest{UserCode: state.UserCode, ClientName: state.ClientName, Status: state.Status.String(), ExpiresAt: state.ExpiresAt}, nil
}
func (s *RedisService) Approve(ctx context.Context, userCode string, ownerID id.ID) error {
	if !ownerID.Valid() {
		return ErrOwnerRequired
	}
	release, err := s.acquire(ctx, "decision:"+strings.ToUpper(strings.TrimSpace(userCode)))
	if err != nil {
		return err
	}
	defer release()
	state, err := s.loadByUserCode(ctx, userCode)
	if err != nil {
		return err
	}
	if state.Status == statusApproved && state.OwnerID == ownerID {
		return nil
	}
	if state.Status != statusPending {
		return ErrAlreadyHandled
	}
	state.Status = statusApproved
	state.OwnerID = ownerID
	return s.saveState(ctx, state)
}
func (s *RedisService) Deny(ctx context.Context, userCode string, ownerID id.ID) error {
	if !ownerID.Valid() {
		return ErrOwnerRequired
	}
	release, err := s.acquire(ctx, "decision:"+strings.ToUpper(strings.TrimSpace(userCode)))
	if err != nil {
		return err
	}
	defer release()
	state, err := s.loadByUserCode(ctx, userCode)
	if err != nil {
		return err
	}
	if state.Status != statusPending {
		return ErrAlreadyHandled
	}
	state.Status = statusDenied
	state.OwnerID = ownerID
	return s.saveState(ctx, state)
}
func (s *RedisService) Exchange(ctx context.Context, deviceCode string) (api.DeviceTokenResponse, error) {
	deviceCode = strings.TrimSpace(deviceCode)
	if deviceCode == "" || len(deviceCode) > 256 {
		return api.DeviceTokenResponse{}, ErrInvalidCode
	}
	deviceHash := hash(deviceCode)
	release, err := s.acquire(ctx, "exchange:"+deviceHash)
	if err != nil {
		return api.DeviceTokenResponse{}, err
	}
	defer release()
	state, err := s.loadByHash(ctx, deviceHash)
	if err != nil {
		return api.DeviceTokenResponse{}, err
	}
	switch state.Status {
	case statusPending, statusExchanging:
		return api.DeviceTokenResponse{}, ErrAuthorizationPending
	case statusDenied:
		return api.DeviceTokenResponse{}, ErrAuthorizationDenied
	case statusApproved:
	default:
		return api.DeviceTokenResponse{}, ErrInvalidCode
	}
	owner, err := s.users.FindByID(ctx, state.OwnerID)
	if err != nil {
		return api.DeviceTokenResponse{}, err
	}
	accessToken, err := randomToken(32)
	if err != nil {
		return api.DeviceTokenResponse{}, ErrTokenGeneration.WithErr(err)
	}
	credential, err := domaincli.NewCredential(state.OwnerID, state.ClientName, hash(accessToken), time.Now().UTC().Add(s.credentialTTL))
	if err != nil {
		return api.DeviceTokenResponse{}, err
	}
	if err := s.credentials.Create(ctx, credential); err != nil {
		return api.DeviceTokenResponse{}, ErrCredentialCreate.WithErr(err)
	}
	if err := s.deleteState(ctx, state); err != nil {
		if cleanupErr := s.credentials.Delete(ctx, credential.ID); cleanupErr != nil {
			log.Warn("failed to rollback CLI credential %s: %v", credential.ID, cleanupErr)
		}
		return api.DeviceTokenResponse{}, ErrStateStorage.WithErr(err)
	}
	return api.DeviceTokenResponse{AccessToken: accessToken, TokenType: "Bearer", ExpiresAt: credential.ExpiresAt, Owner: api.User{ID: owner.ID.String(), Email: owner.Email, Name: owner.Name, AvatarURL: owner.AvatarURL}}, nil
}
func (s *RedisService) Authenticate(ctx context.Context, token string) (*user.User, *domaincli.Credential, error) {
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
func (s *RedisService) Revoke(ctx context.Context, credentialID id.ID) error {
	err := s.credentials.Delete(ctx, credentialID)
	if stderrors.Is(err, domaincli.ErrCredentialNotFound) {
		return domaincli.ErrCredentialNotFound
	}
	return err
}
