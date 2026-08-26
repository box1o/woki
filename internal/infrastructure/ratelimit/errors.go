package ratelimit

import "github.com/box1o/woki/pkg/errors"

var (
	ErrBackend       = errors.New("RATE_LIMIT_BACKEND_FAILED", "rate-limit backend failed")
	ErrInvalidPolicy = errors.New("RATE_LIMIT_POLICY_INVALID", "rate-limit policy is invalid")
)
