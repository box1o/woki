package session

import (
	"context"
	"errors"
	"strings"
	"testing"
	"time"

	"github.com/box1o/woki/pkg/id"
)

func TestSessionExpiresAbsolutely(t *testing.T) {
	m := New(30 * time.Millisecond)
	uid := id.New()
	token, err := m.Create(context.Background(), uid)
	if err != nil {
		t.Fatal(err)
	}
	time.Sleep(20 * time.Millisecond)
	if _, err := m.Get(context.Background(), token); err != nil {
		t.Fatal(err)
	}
	time.Sleep(20 * time.Millisecond)
	if _, err := m.Get(context.Background(), token); err == nil {
		t.Fatal("session lifetime was unexpectedly extended")
	}
}

func TestSessionRejectsMalformedToken(t *testing.T) {
	m := New(time.Hour)
	if _, err := m.Get(context.Background(), ""); !errors.Is(err, ErrNotFound) {
		t.Fatalf("Get(empty)=%v; want ErrNotFound", err)
	}
	if err := m.Delete(context.Background(), strings.Repeat("x", 1024)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("Delete(oversized)=%v; want ErrNotFound", err)
	}
}
