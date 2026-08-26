package middleware

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrSessionMissing = apperrors.New("AUTH_SESSION_REQUIRED", "browser session is required")
	ErrBearerMissing  = apperrors.New("CLI_CREDENTIAL_REQUIRED", "CLI bearer credential is required")
)
