package user

import (
	"net/mail"
	"strings"
	"time"

	"github.com/box1o/woki/pkg/id"
)

const (
	maxEmailLength      = 254
	maxNameLength       = 100
	maxProviderIDLength = 255
)

type Provider string

const (
	ProviderGitHub Provider = "github"
	ProviderGoogle Provider = "google"
	ProviderDev    Provider = "dev"
)

type User struct {
	ID         id.ID     `json:"id"`
	Email      string    `json:"email"`
	Name       string    `json:"name"`
	AvatarURL  string    `json:"avatar_url,omitempty"`
	Provider   Provider  `json:"provider"`
	ProviderID string    `json:"provider_id"`
	CreatedAt  time.Time `json:"created_at"`
	UpdatedAt  time.Time `json:"updated_at"`
}

func New(email, name, avatarURL string, provider Provider, providerID string) (*User, error) {
	now := time.Now().UTC()
	u := &User{
		ID:         id.New(),
		Email:      normalizeEmail(email),
		Name:       strings.TrimSpace(name),
		AvatarURL:  strings.TrimSpace(avatarURL),
		Provider:   provider,
		ProviderID: strings.TrimSpace(providerID),
		CreatedAt:  now,
		UpdatedAt:  now,
	}
	if err := u.Validate(); err != nil {
		return nil, err
	}
	return u, nil
}

func (u *User) UpdateProfile(email, name, avatarURL string, provider Provider, providerID string) error {
	next := *u
	next.Email = normalizeEmail(email)
	next.Name = strings.TrimSpace(name)
	next.AvatarURL = strings.TrimSpace(avatarURL)
	next.Provider = provider
	next.ProviderID = strings.TrimSpace(providerID)
	if err := next.Validate(); err != nil {
		return err
	}
	next.UpdatedAt = time.Now().UTC()
	*u = next
	return nil
}

func (u User) Validate() error {
	if !u.ID.Valid() {
		return ErrNotFound
	}
	if err := validateEmail(u.Email); err != nil {
		return err
	}
	name := strings.TrimSpace(u.Name)
	if name == "" {
		return ErrNameEmpty
	}
	if len(name) > maxNameLength {
		return ErrNameTooLong
	}
	if u.Provider == "" {
		return ErrProviderEmpty
	}
	switch u.Provider {
	case ProviderGitHub, ProviderGoogle, ProviderDev:
	default:
		return ErrProviderInvalid
	}
	providerID := strings.TrimSpace(u.ProviderID)
	if providerID == "" {
		return ErrProviderIDEmpty
	}
	if len(providerID) > maxProviderIDLength {
		return ErrProviderIDTooLong
	}
	return nil
}

func validateEmail(email string) error {
	email = normalizeEmail(email)
	if email == "" {
		return ErrEmailInvalid
	}
	if len(email) > maxEmailLength {
		return ErrEmailTooLong
	}
	address, err := mail.ParseAddress(email)
	if err != nil || !strings.EqualFold(address.Address, email) {
		return ErrEmailInvalid
	}
	return nil
}

func normalizeEmail(email string) string {
	return strings.ToLower(strings.TrimSpace(email))
}
