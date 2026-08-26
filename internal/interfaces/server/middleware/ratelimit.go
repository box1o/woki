package middleware

import (
	"net"
	"net/http"
	"strconv"
	"strings"
	"time"

	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/log"
	contract "github.com/box1o/woki/pkg/ratelimit"
)

func RateLimit(limiter contract.Limiter, namespace string, policy contract.Policy, failClosed bool) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		if limiter == nil || !policy.Valid() {
			return next
		}
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			key := strings.Trim(namespace, ":") + ":" + RateLimitIdentity(r)
			decision, err := limiter.Allow(r.Context(), key, policy)
			if err != nil {
				log.Error("rate limiter %s failed: %v", namespace, err)
				if failClosed {
					apperrors.WriteError(w, apperrors.ErrServiceUnavailable.WithMessage("Rate limiter unavailable"))
					return
				}
				next.ServeHTTP(w, r)
				return
			}

			WriteRateLimitHeaders(w, namespace, policy, decision)
			if !decision.Allowed {
				w.Header().Set("Retry-After", strconv.Itoa(durationSeconds(decision.RetryAfter)))
				apperrors.WriteError(w, apperrors.NewHTTP(http.StatusTooManyRequests, "RATE_LIMITED", "Too many requests"))
				return
			}
			next.ServeHTTP(w, r)
		})
	}
}

func WriteRateLimitHeaders(w http.ResponseWriter, _ string, policy contract.Policy, decision contract.Decision) {
	limit := strconv.Itoa(policy.Rate)
	remaining := strconv.Itoa(maxInt(decision.Remaining, 0))
	reset := strconv.Itoa(durationSeconds(decision.ResetAfter))
	burst := strconv.Itoa(policy.Burst)

	// RateLimit-* is the widely deployed response shape used by current API
	// clients. X-RateLimit-* is kept for older consumers. Burst is Woki-specific
	// because the standard fields describe the sustained quota, not GCRA burst.
	w.Header().Set("RateLimit-Limit", limit)
	w.Header().Set("RateLimit-Remaining", remaining)
	w.Header().Set("RateLimit-Reset", reset)
	w.Header().Set("X-RateLimit-Limit", limit)
	w.Header().Set("X-RateLimit-Remaining", remaining)
	w.Header().Set("X-RateLimit-Reset", reset)
	w.Header().Set("X-RateLimit-Burst", burst)
}

func RateLimitIdentity(r *http.Request) string {
	if principal, ok := PrincipalFrom(r.Context()); ok && principal.User != nil {
		return "user:" + principal.User.ID.String()
	}
	return "ip:" + ClientIP(r)
}

func ClientIP(r *http.Request) string {
	host, _, err := net.SplitHostPort(strings.TrimSpace(r.RemoteAddr))
	if err == nil && host != "" {
		return host
	}
	if value := strings.TrimSpace(r.RemoteAddr); value != "" {
		return value
	}
	return "unknown"
}

func durationSeconds(value time.Duration) int {
	if value <= 0 {
		return 0
	}
	seconds := int((value + time.Second - 1) / time.Second)
	if seconds < 1 {
		return 1
	}
	return seconds
}

func maxInt(value, minimum int) int {
	if value < minimum {
		return minimum
	}
	return value
}
