package ratelimit

import (
	"context"
	"errors"
	"testing"
	"time"

	contract "github.com/box1o/woki/pkg/ratelimit"
)

func TestMemoryGCRASmoothsBursts(t *testing.T) {
	now := time.Unix(1000, 0)
	limiter := newMemory(func() time.Time { return now })
	policy := contract.Policy{Rate: 10, Burst: 2, Period: time.Second}

	first, err := limiter.Allow(context.Background(), "client", policy)
	if err != nil || !first.Allowed || first.Remaining != 1 {
		t.Fatalf("first request = %+v, %v", first, err)
	}
	second, err := limiter.Allow(context.Background(), "client", policy)
	if err != nil || !second.Allowed || second.Remaining != 0 {
		t.Fatalf("second request = %+v, %v", second, err)
	}
	third, err := limiter.Allow(context.Background(), "client", policy)
	if err != nil {
		t.Fatal(err)
	}
	if third.Allowed {
		t.Fatalf("third request should be throttled: %+v", third)
	}
	if third.RetryAfter != 100*time.Millisecond {
		t.Fatalf("retry after = %s, want 100ms", third.RetryAfter)
	}

	now = now.Add(100 * time.Millisecond)
	next, err := limiter.Allow(context.Background(), "client", policy)
	if err != nil || !next.Allowed {
		t.Fatalf("request after refill = %+v, %v", next, err)
	}
}

func TestMemoryGCRANoFixedWindowBoundaryBurst(t *testing.T) {
	now := time.Unix(2000, 0)
	limiter := newMemory(func() time.Time { return now })
	policy := contract.Policy{Rate: 4, Burst: 2, Period: time.Second}

	for i := 0; i < 2; i++ {
		decision, err := limiter.Allow(context.Background(), "client", policy)
		if err != nil || !decision.Allowed {
			t.Fatalf("burst request %d = %+v, %v", i, decision, err)
		}
	}

	now = now.Add(249 * time.Millisecond)
	decision, err := limiter.Allow(context.Background(), "client", policy)
	if err != nil {
		t.Fatal(err)
	}
	if decision.Allowed {
		t.Fatalf("request before emission interval should be blocked: %+v", decision)
	}

	now = now.Add(time.Millisecond)
	decision, err = limiter.Allow(context.Background(), "client", policy)
	if err != nil || !decision.Allowed {
		t.Fatalf("request at emission interval = %+v, %v", decision, err)
	}
}

func TestMemoryRejectsInvalidPolicy(t *testing.T) {
	limiter := NewMemory()
	_, err := limiter.Allow(context.Background(), "client", contract.Policy{Rate: 10, Burst: 0, Period: time.Second})
	if !errors.Is(err, ErrInvalidPolicy) {
		t.Fatalf("error = %v, want %v", err, ErrInvalidPolicy)
	}
}
