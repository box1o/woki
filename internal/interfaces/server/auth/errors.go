package auth

import (
	apperrors "github.com/box1o/woki/pkg/errors"
	"net/http"
)

var (
	ErrOAuthNotConfigured = apperrors.NewHTTP(http.StatusServiceUnavailable, "OAUTH_NOT_CONFIGURED", "Authentication provider is not configured")
	ErrOAuthStateInvalid  = apperrors.NewHTTP(http.StatusBadRequest, "OAUTH_STATE_INVALID", "OAuth state validation failed")
	ErrOAuthCodeRequired  = apperrors.NewHTTP(http.StatusBadRequest, "OAUTH_CODE_REQUIRED", "OAuth authorization code is required")
	ErrOAuthFailed        = apperrors.NewHTTP(http.StatusBadGateway, "OAUTH_AUTHENTICATION_FAILED", "OAuth authentication failed")
	ErrAuthentication     = apperrors.NewHTTP(http.StatusInternalServerError, "AUTHENTICATION_FAILED", "Authentication failed")
	ErrDevAuthDisabled    = apperrors.NewHTTP(http.StatusNotFound, "NOT_FOUND", "Resource not found")
	// Compatibility aliases for older callers/tests.
	ErrGitHubNotConfigured = ErrOAuthNotConfigured
	ErrGitHubAuthFailed    = ErrOAuthFailed
)
