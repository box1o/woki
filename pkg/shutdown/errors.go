package shutdown

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrInvalidTimeout = apperrors.New("SHUTDOWN_TIMEOUT_INVALID", "shutdown timeout must be positive")
	ErrFailed         = apperrors.New("SHUTDOWN_FAILED", "graceful shutdown failed")
)
