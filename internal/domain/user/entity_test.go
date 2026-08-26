package user

import (
	"errors"
	"strings"
	"testing"
)

func TestNewNormalizesUser(t *testing.T) {
	u, err := New(
		" User@Example.COM ",
		" User ",
		" avatar ",
		ProviderDev,
		"dev-user",
	)
	if err != nil {
		t.Fatal(err)
	}
	if u.Email != "user@example.com" ||
		u.Name != "User" ||
		u.AvatarURL != "avatar" ||
		u.ProviderID != "dev-user" ||
		!u.ID.Valid() {
		t.Fatalf("unexpected user: %+v", u)
	}
}

func TestNewRejectsInvalidUser(t *testing.T) {
	for name, tc := range map[string]struct {
		email      string
		name       string
		provider   Provider
		providerID string
		want       error
	}{
		"email": {
			email: "not-an-email", name: "User", provider: ProviderDev,
			providerID: "id", want: ErrEmailInvalid,
		},
		"name": {
			email: "user@example.com", name: "", provider: ProviderDev,
			providerID: "id", want: ErrNameEmpty,
		},
		"long name": {
			email: "user@example.com", name: strings.Repeat("x", 101), provider: ProviderDev,
			providerID: "id", want: ErrNameTooLong,
		},
		"provider": {
			email: "user@example.com", name: "User", provider: Provider("other"),
			providerID: "id", want: ErrProviderInvalid,
		},
		"provider ID": {
			email: "user@example.com", name: "User", provider: ProviderDev,
			providerID: "", want: ErrProviderIDEmpty,
		},
	} {
		t.Run(name, func(t *testing.T) {
			_, err := New(tc.email, tc.name, "", tc.provider, tc.providerID)
			if !errors.Is(err, tc.want) {
				t.Fatalf("New error=%v; want %v", err, tc.want)
			}
		})
	}
}

func TestUpdateProfileDoesNotPartiallyMutateOnValidationFailure(t *testing.T) {
	u, err := New("user@example.com", "User", "", ProviderDev, "id")
	if err != nil {
		t.Fatal(err)
	}
	before := *u
	if err := u.UpdateProfile("invalid", "Changed", "", ProviderDev, "id"); err == nil {
		t.Fatal("UpdateProfile unexpectedly succeeded")
	}
	if *u != before {
		t.Fatalf("invalid update mutated user: before=%+v after=%+v", before, *u)
	}
}
