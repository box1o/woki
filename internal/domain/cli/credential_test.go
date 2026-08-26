package cli

import (
	"errors"
	"testing"
	"time"

	"github.com/box1o/woki/pkg/id"
)

func TestCredentialExpiry(t *testing.T) {
	c, err := NewCredential(id.New(), "Woki CLI", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", time.Now().Add(time.Minute))
	if err != nil {
		t.Fatal(err)
	}
	if err := c.Check(time.Now()); err != nil {
		t.Fatal(err)
	}
	if err := c.Check(c.ExpiresAt); !errors.Is(err, ErrCredentialExpired) {
		t.Fatalf("got %v", err)
	}
}
