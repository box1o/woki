package deviceauth

import (
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"strings"
	"time"
)

func (s *Service) stateByUserCodeLocked(userCode string) (*deviceState, error) {
	deviceHash, ok := s.byUserCode[strings.ToUpper(strings.TrimSpace(userCode))]
	if !ok {
		return nil, ErrAuthorizationExpired
	}
	state := s.byDevice[deviceHash]
	if state == nil {
		return nil, ErrAuthorizationExpired
	}
	return state, nil
}

func (s *Service) resetExchange(deviceHash string) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if state := s.byDevice[deviceHash]; state != nil && state.Status == statusExchanging {
		state.Status = statusApproved
	}
}

func (s *Service) cleanupLocked(now time.Time) {
	for _, state := range s.byDevice {
		if !now.Before(state.ExpiresAt) {
			s.deleteStateLocked(state)
		}
	}
}

func (s *Service) deleteStateLocked(state *deviceState) {
	delete(s.byUserCode, state.UserCode)
	delete(s.byDevice, state.DeviceHash)
}

func (s *Service) uniqueDeviceTokenLocked() (string, string, error) {
	for range 8 {
		token, err := randomToken(32)
		if err != nil {
			return "", "", ErrCodeGenerationFailed.WithErr(err)
		}
		tokenHash := hash(token)
		if _, exists := s.byDevice[tokenHash]; !exists {
			return token, tokenHash, nil
		}
	}
	return "", "", ErrCodeGenerationFailed.WithDetail("exhausted device-code generation attempts")
}

func (s *Service) uniqueUserCodeLocked() (string, error) {
	const alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
	for range 32 {
		code, err := randomCode(alphabet, 8)
		if err != nil {
			return "", ErrCodeGenerationFailed.WithErr(err)
		}
		if _, exists := s.byUserCode[code]; !exists {
			return code, nil
		}
	}
	return "", ErrCodeGenerationFailed.WithDetail("exhausted user-code generation attempts")
}

func randomCode(alphabet string, length int) (string, error) {
	out := make([]byte, length)
	limit := 256 - (256 % len(alphabet))
	for i := 0; i < length; {
		var random [1]byte
		if _, err := rand.Read(random[:]); err != nil {
			return "", err
		}
		if int(random[0]) >= limit {
			continue
		}
		out[i] = alphabet[int(random[0])%len(alphabet)]
		i++
	}
	return string(out), nil
}

func randomToken(size int) (string, error) {
	buf := make([]byte, size)
	if _, err := rand.Read(buf); err != nil {
		return "", err
	}
	return base64.RawURLEncoding.EncodeToString(buf), nil
}

func hash(value string) string {
	sum := sha256.Sum256([]byte(value))
	return hex.EncodeToString(sum[:])
}
