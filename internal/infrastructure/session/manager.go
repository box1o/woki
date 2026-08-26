// Package session owns short-lived browser sessions.
package session

import (
	"context"
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"sync"
	"time"

	"github.com/box1o/woki/pkg/id"
)

const sessionTokenLength = 43 // base64.RawURLEncoding of 32 random bytes

type entry struct {
	UserID    id.ID
	ExpiresAt time.Time
}

type Manager struct {
	mu       sync.Mutex
	ttl      time.Duration
	sessions map[[32]byte]entry
}

func New(ttl time.Duration) *Manager { return &Manager{ttl: ttl, sessions: make(map[[32]byte]entry)} }

func (m *Manager) Create(_ context.Context, userID id.ID) (string, error) {
	if !userID.Valid() {
		return "", ErrUserRequired
	}
	token, err := randomToken(32)
	if err != nil {
		return "", ErrTokenGeneration.WithErr(err)
	}
	hash := sha256.Sum256([]byte(token))
	m.mu.Lock()
	m.cleanupLocked(time.Now())
	m.sessions[hash] = entry{UserID: userID, ExpiresAt: time.Now().UTC().Add(m.ttl)}
	m.mu.Unlock()
	return token, nil
}
func (m *Manager) Get(_ context.Context, token string) (id.ID, error) {
	if len(token) != sessionTokenLength {
		return "", ErrNotFound
	}
	hash := sha256.Sum256([]byte(token))
	now := time.Now()
	m.mu.Lock()
	defer m.mu.Unlock()
	m.cleanupLocked(now)
	entry, ok := m.sessions[hash]
	if !ok {
		return "", ErrNotFound
	}
	return entry.UserID, nil
}
func (m *Manager) Delete(_ context.Context, token string) error {
	if len(token) != sessionTokenLength {
		return ErrNotFound
	}
	hash := sha256.Sum256([]byte(token))
	m.mu.Lock()
	defer m.mu.Unlock()
	if _, ok := m.sessions[hash]; !ok {
		return ErrNotFound
	}
	delete(m.sessions, hash)
	return nil
}
func (m *Manager) cleanupLocked(now time.Time) {
	for hash, v := range m.sessions {
		if !now.Before(v.ExpiresAt) {
			delete(m.sessions, hash)
		}
	}
}
func randomToken(bytes int) (string, error) {
	buf := make([]byte, bytes)
	if _, err := rand.Read(buf); err != nil {
		return "", err
	}
	return base64.RawURLEncoding.EncodeToString(buf), nil
}
