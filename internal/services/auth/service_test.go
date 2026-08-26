package auth

import (
	"context"
	"errors"
	"testing"
	"time"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/infrastructure/memory"
	"github.com/box1o/woki/internal/infrastructure/session"
)

func TestLoginCreatesAndUpdatesUser(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	repo := memory.UserRepository{Store: store}
	sessions := session.New(time.Hour)
	ensures := 0
	svc := New(repo, sessions, func(context.Context, *user.User) error {
		ensures++
		return nil
	})

	u, token, err := svc.Login(ctx, Profile{
		Email:    "USER@example.com",
		Name:     "First",
		Provider: user.ProviderDev,
	})
	if err != nil {
		t.Fatal(err)
	}
	if token == "" || u.Email != "user@example.com" {
		t.Fatalf("unexpected login: %+v %q", u, token)
	}

	u2, _, err := svc.Login(ctx, Profile{
		Email:    "user@example.com",
		Name:     "Second",
		Provider: user.ProviderDev,
	})
	if err != nil {
		t.Fatal(err)
	}
	if u2.ID != u.ID || u2.Name != "Second" || ensures != 2 {
		t.Fatalf("unexpected update: %+v ensures=%d", u2, ensures)
	}
}

func TestLoginUsesStableProviderIdentity(t *testing.T) {
	ctx := context.Background()
	repo := memory.UserRepository{Store: memory.New()}
	svc := New(repo, session.New(time.Hour), func(context.Context, *user.User) error {
		return nil
	})

	first, _, err := svc.Login(ctx, Profile{
		Email:      "old@example.com",
		Name:       "User",
		Provider:   user.ProviderGitHub,
		ProviderID: "12345",
	})
	if err != nil {
		t.Fatal(err)
	}
	second, _, err := svc.Login(ctx, Profile{
		Email:      "new@example.com",
		Name:       "User",
		Provider:   user.ProviderGitHub,
		ProviderID: "12345",
	})
	if err != nil {
		t.Fatal(err)
	}
	if second.ID != first.ID || second.Email != "new@example.com" {
		t.Fatalf("provider identity was not preserved: first=%+v second=%+v", first, second)
	}
}

func TestLoginDoesNotLinkDifferentIdentityByEmail(t *testing.T) {
	ctx := context.Background()
	repo := memory.UserRepository{Store: memory.New()}
	svc := New(repo, session.New(time.Hour), func(context.Context, *user.User) error {
		return nil
	})

	if _, _, err := svc.Login(ctx, Profile{
		Email:      "user@example.com",
		Name:       "User",
		Provider:   user.ProviderGitHub,
		ProviderID: "github-1",
	}); err != nil {
		t.Fatal(err)
	}
	_, _, err := svc.Login(ctx, Profile{
		Email:      "user@example.com",
		Name:       "Other",
		Provider:   user.ProviderGitHub,
		ProviderID: "github-2",
	})
	if !errors.Is(err, ErrIdentityConflict) {
		t.Fatalf("Login error=%v; want ErrIdentityConflict", err)
	}
}
