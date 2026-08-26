package cli

import (
	"encoding/hex"
	"strings"
	"time"

	"github.com/box1o/woki/pkg/id"
)

const maxClientNameLength = 100

type Credential struct {
	ID         id.ID     `json:"id"`
	OwnerID    id.ID     `json:"owner_id"`
	ClientName string    `json:"client_name"`
	TokenHash  string    `json:"token_hash"`
	CreatedAt  time.Time `json:"created_at"`
	ExpiresAt  time.Time `json:"expires_at"`
}

func NewCredential(
	ownerID id.ID,
	clientName, tokenHash string,
	expiresAt time.Time,
) (*Credential, error) {
	now := time.Now().UTC()
	c := &Credential{
		ID:         id.New(),
		OwnerID:    ownerID,
		ClientName: strings.TrimSpace(clientName),
		TokenHash:  strings.ToLower(strings.TrimSpace(tokenHash)),
		CreatedAt:  now,
		ExpiresAt:  expiresAt.UTC(),
	}
	if err := c.Validate(); err != nil {
		return nil, err
	}
	return c, nil
}

func (c Credential) Validate() error {
	if !c.ID.Valid() {
		return ErrCredentialNotFound
	}
	if !c.OwnerID.Valid() {
		return ErrOwnerRequired
	}
	clientName := strings.TrimSpace(c.ClientName)
	if clientName == "" {
		return ErrClientNameEmpty
	}
	if len(clientName) > maxClientNameLength {
		return ErrClientNameTooLong
	}
	if !validTokenHash(c.TokenHash) {
		return ErrTokenHashInvalid
	}
	if !c.ExpiresAt.After(c.CreatedAt) {
		return ErrInvalidExpiry
	}
	return nil
}

func (c Credential) Check(now time.Time) error {
	if !now.Before(c.ExpiresAt) {
		return ErrCredentialExpired
	}
	return nil
}

func validTokenHash(value string) bool {
	value = strings.TrimSpace(value)
	if len(value) != 64 {
		return false
	}
	_, err := hex.DecodeString(value)
	return err == nil
}
