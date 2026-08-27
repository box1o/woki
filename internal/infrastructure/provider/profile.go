package provider

import "github.com/box1o/woki/internal/domain/user"

// Profile is the verified identity returned by an external sign-in provider.
type Profile struct {
	Email      string
	Name       string
	AvatarURL  string
	Provider   user.Provider
	ProviderID string
}
