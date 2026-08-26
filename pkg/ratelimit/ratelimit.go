package ratelimit

import (
	"context"
	"time"
)

// Policy describes a sustained request rate with a bounded instantaneous burst.
// Rate events are permitted during Period, while Burst controls how many may be
// admitted immediately before the limiter starts smoothing traffic.
type Policy struct {
	Rate   int
	Burst  int
	Period time.Duration
}

func (p Policy) Valid() bool {
	return p.Rate > 0 && p.Burst > 0 && p.Period > 0
}

type Decision struct {
	Allowed    bool
	Remaining  int
	RetryAfter time.Duration
	ResetAfter time.Duration
}

type Limiter interface {
	Allow(context.Context, string, Policy) (Decision, error)
}
