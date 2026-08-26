package deviceauth

import (
	"context"
	"errors"
	"testing"
	"time"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/infrastructure/memory"
)

func TestDeviceAuthorizationFlow(t *testing.T) {
	ctx := context.Background()
	store := memory.New()
	users := memory.UserRepository{Store: store}
	credentials := memory.CredentialRepository{Store: store}
	owner, _ := user.New("owner@example.com", "Owner", "", user.ProviderDev, "owner@example.com")
	_ = users.Create(ctx, owner)
	svc := New("http://localhost:5173", time.Minute, time.Hour, credentials, users)
	code, err := svc.Create(ctx, "test cli")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := svc.Exchange(ctx, code.DeviceCode); !errors.Is(err, ErrAuthorizationPending) {
		t.Fatalf("got %v", err)
	}
	if err := svc.Approve(ctx, code.UserCode, owner.ID); err != nil {
		t.Fatal(err)
	}
	token, err := svc.Exchange(ctx, code.DeviceCode)
	if err != nil {
		t.Fatal(err)
	}
	if token.AccessToken == "" {
		t.Fatal("empty token")
	}
	got, credential, err := svc.Authenticate(ctx, token.AccessToken)
	if err != nil {
		t.Fatal(err)
	}
	if got.ID != owner.ID || credential.OwnerID != owner.ID {
		t.Fatalf("unexpected authentication result")
	}
	if err := svc.Revoke(ctx, credential.ID); err != nil {
		t.Fatal(err)
	}
	if _, _, err := svc.Authenticate(ctx, token.AccessToken); !errors.Is(err, domaincli.ErrCredentialNotFound) {
		t.Fatalf("Authenticate after revoke error = %v; want %v", err, domaincli.ErrCredentialNotFound)
	}
}
