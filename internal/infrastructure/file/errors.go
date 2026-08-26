package file

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrPathRequired   = apperrors.New("STORAGE_PATH_REQUIRED", "storage data-file path is required")
	ErrReadFailed     = apperrors.New("STORAGE_READ_FAILED", "failed to read storage state")
	ErrStateEmpty     = apperrors.New("STORAGE_STATE_INVALID", "storage state file is empty")
	ErrStateTooLarge  = apperrors.New("STORAGE_STATE_TOO_LARGE", "storage state file is unexpectedly large")
	ErrDecodeFailed   = apperrors.New("STORAGE_DECODE_FAILED", "failed to decode storage state")
	ErrStateInvalid   = apperrors.New("STORAGE_STATE_INVALID", "storage state is invalid")
	ErrPersistFailed  = apperrors.New("STORAGE_PERSIST_FAILED", "failed to persist storage state")
	ErrDurability     = apperrors.New("STORAGE_DURABILITY_FAILED", "storage state was committed but durability verification failed")
	ErrRollbackFailed = apperrors.New("STORAGE_ROLLBACK_FAILED", "failed to restore storage state after a persistence failure")
)
