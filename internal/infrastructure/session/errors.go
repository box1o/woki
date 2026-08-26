package session

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrStorage         = apperrors.New("SESSION_STORAGE_FAILED", "session storage operation failed")
	ErrNotFound        = apperrors.New("SESSION_NOT_FOUND", "session not found or expired")
	ErrUserRequired    = apperrors.New("SESSION_USER_REQUIRED", "session user is required")
	ErrTokenGeneration = apperrors.New("SESSION_TOKEN_GENERATION_FAILED", "failed to generate session token")
)
