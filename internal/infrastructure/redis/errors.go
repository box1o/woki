package redis

import "github.com/box1o/woki/pkg/errors"

var (
	ErrConnection = errors.New("REDIS_CONNECTION_FAILED", "Redis connection failed")
	ErrOperation  = errors.New("REDIS_OPERATION_FAILED", "Redis operation failed")
	ErrNotFound   = errors.New("REDIS_NOT_FOUND", "Redis key was not found")
)
