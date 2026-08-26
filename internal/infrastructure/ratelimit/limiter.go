package ratelimit

import (
	"context"
	"sync"
	"time"

	contract "github.com/box1o/woki/pkg/ratelimit"
)

type memoryCell struct {
	tat      time.Time
	lastSeen time.Time
}

// Memory implements GCRA in process memory. It intentionally mirrors the
// Redis-backed limiter semantics so development and Redis failover do not use a
// weaker fixed-window algorithm.
type Memory struct {
	mu          sync.Mutex
	items       map[string]memoryCell
	now         func() time.Time
	lastCleanup time.Time
}

func NewMemory() *Memory {
	return newMemory(time.Now)
}

func newMemory(now func() time.Time) *Memory {
	current := now()
	return &Memory{
		items:       make(map[string]memoryCell),
		now:         now,
		lastCleanup: current,
	}
}

func (l *Memory) Allow(_ context.Context, key string, policy contract.Policy) (contract.Decision, error) {
	if !policy.Valid() {
		return contract.Decision{}, ErrInvalidPolicy
	}

	emission := policy.Period / time.Duration(policy.Rate)
	if emission <= 0 {
		return contract.Decision{}, ErrInvalidPolicy.WithDetail("rate is too high for the configured period")
	}

	now := l.now()
	l.mu.Lock()
	defer l.mu.Unlock()

	cell := l.items[key]
	tat := cell.tat
	if tat.Before(now) {
		tat = now
	}

	newTAT := tat.Add(emission)
	allowAt := newTAT.Add(-emission * time.Duration(policy.Burst))
	if now.Before(allowAt) {
		l.cleanupLocked(now, policy.Period)
		return contract.Decision{
			Allowed:    false,
			Remaining:  0,
			RetryAfter: allowAt.Sub(now),
			ResetAfter: maxDuration(tat.Sub(now), 0),
		}, nil
	}

	remaining := int(now.Sub(allowAt) / emission)
	if remaining < 0 {
		remaining = 0
	}
	if remaining > policy.Burst-1 {
		remaining = policy.Burst - 1
	}

	l.items[key] = memoryCell{tat: newTAT, lastSeen: now}
	l.cleanupLocked(now, policy.Period)

	return contract.Decision{
		Allowed:    true,
		Remaining:  remaining,
		RetryAfter: -1,
		ResetAfter: maxDuration(newTAT.Sub(now), 0),
	}, nil
}

func (l *Memory) cleanupLocked(now time.Time, period time.Duration) {
	if len(l.items) < 4096 && now.Sub(l.lastCleanup) < time.Minute {
		return
	}
	l.lastCleanup = now
	cutoff := now.Add(-period)
	for key, cell := range l.items {
		if !cell.tat.After(now) && cell.lastSeen.Before(cutoff) {
			delete(l.items, key)
		}
	}
}

func maxDuration(value, minimum time.Duration) time.Duration {
	if value < minimum {
		return minimum
	}
	return value
}
