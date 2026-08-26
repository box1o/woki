package log

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrInvalidLevel  = apperrors.New("LOG_LEVEL_INVALID", "log level is invalid")
	ErrInvalidOutput = apperrors.New("LOG_OUTPUT_INVALID", "log output is invalid")
	ErrFileRequired  = apperrors.New("LOG_FILE_REQUIRED", "log file path is required")
	ErrFileOpen      = apperrors.New("LOG_FILE_OPEN_FAILED", "failed to open log file")
	ErrDirectory     = apperrors.New("LOG_DIRECTORY_FAILED", "failed to prepare log directory")
	ErrCleanup       = apperrors.New("LOG_CLEANUP_FAILED", "failed to close log output")
)
