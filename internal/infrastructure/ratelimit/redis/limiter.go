package redis

import (
	"context"
	stderrors "errors"
	"sync"
	"time"

	infraratelimit "github.com/box1o/woki/internal/infrastructure/ratelimit"
	"github.com/box1o/woki/pkg/log"
	contract "github.com/box1o/woki/pkg/ratelimit"
	redisrate "github.com/go-redis/redis_rate/v10"
	goredis "github.com/redis/go-redis/v9"
)

const warningInterval = 30 * time.Second

// Limiter uses go-redis/redis_rate's atomic GCRA implementation as the shared
// distributed limiter and falls back to the same policy in local memory if
// Redis is temporarily unavailable.
type Limiter struct {
	primary  *redisrate.Limiter
	fallback contract.Limiter

	warningMu   sync.Mutex
	lastWarning time.Time
}

func New(client *goredis.Client, fallback contract.Limiter) *Limiter {
	return &Limiter{
		primary:  redisrate.NewLimiter(client),
		fallback: fallback,
	}
}

func (l *Limiter) Allow(ctx context.Context, key string, policy contract.Policy) (contract.Decision, error) {
	if !policy.Valid() {
		return contract.Decision{}, infraratelimit.ErrInvalidPolicy
	}

	result, err := l.primary.Allow(ctx, key, redisrate.Limit{
		Rate:   policy.Rate,
		Burst:  policy.Burst,
		Period: policy.Period,
	})
	if err != nil {
		if stderrors.Is(err, context.Canceled) || stderrors.Is(err, context.DeadlineExceeded) || ctx.Err() != nil {
			return contract.Decision{}, err
		}
		if l.fallback == nil {
			return contract.Decision{}, infraratelimit.ErrBackend.WithErr(err)
		}
		l.warnFallback(err)
		return l.fallback.Allow(ctx, key, policy)
	}

	return contract.Decision{
		Allowed:    result.Allowed > 0,
		Remaining:  result.Remaining,
		RetryAfter: result.RetryAfter,
		ResetAfter: result.ResetAfter,
	}, nil
}

func (l *Limiter) warnFallback(err error) {
	now := time.Now()
	l.warningMu.Lock()
	defer l.warningMu.Unlock()
	if !l.lastWarning.IsZero() && now.Sub(l.lastWarning) < warningInterval {
		return
	}
	l.lastWarning = now
	log.Warn("Redis rate limiter unavailable; using process-local GCRA fallback: %v", err)
}
