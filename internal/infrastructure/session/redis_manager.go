package session

import (
	"context"
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	stderrors "errors"
	"strings"
	"time"

	infraredis "github.com/box1o/woki/internal/infrastructure/redis"
	"github.com/box1o/woki/pkg/id"
)

type RedisManager struct {
	redis  *infraredis.Client
	ttl    time.Duration
	prefix string
}

func NewRedis(redis *infraredis.Client, ttl time.Duration, prefix string) *RedisManager {
	return &RedisManager{redis: redis, ttl: ttl, prefix: strings.Trim(strings.TrimSpace(prefix), ":")}
}

func (m *RedisManager) key(token string) string {
	sum := sha256.Sum256([]byte(token))
	return m.redis.Key(m.prefix, hex.EncodeToString(sum[:]))
}

func (m *RedisManager) Create(ctx context.Context, userID id.ID) (string, error) {
	if !userID.Valid() {
		return "", ErrUserRequired
	}
	buf := make([]byte, 32)
	if _, err := rand.Read(buf); err != nil {
		return "", ErrTokenGeneration.WithErr(err)
	}
	token := base64.RawURLEncoding.EncodeToString(buf)
	if err := m.redis.Set(ctx, m.key(token), userID.String(), m.ttl); err != nil {
		return "", ErrStorage.WithErr(err)
	}
	return token, nil
}
func (m *RedisManager) Get(ctx context.Context, token string) (id.ID, error) {
	if len(strings.TrimSpace(token)) != sessionTokenLength {
		return "", ErrNotFound
	}
	value, err := m.redis.Get(ctx, m.key(token))
	if err != nil {
		if stderrors.Is(err, infraredis.ErrNotFound) {
			return "", ErrNotFound.WithErr(err)
		}
		return "", ErrStorage.WithErr(err)
	}
	uid, err := id.Parse(value)
	if err != nil {
		return "", ErrNotFound.WithErr(err)
	}
	return uid, nil
}
func (m *RedisManager) Delete(ctx context.Context, token string) error {
	if len(strings.TrimSpace(token)) != sessionTokenLength {
		return ErrNotFound
	}
	if err := m.redis.Delete(ctx, m.key(token)); err != nil {
		return ErrStorage.WithErr(err)
	}
	return nil
}
