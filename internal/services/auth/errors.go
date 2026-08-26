package auth

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrUserLookupFailed   = apperrors.New("AUTH_USER_LOOKUP_FAILED", "failed to resolve authenticated user")
	ErrUserUpdateFailed   = apperrors.New("AUTH_USER_UPDATE_FAILED", "failed to update authenticated user")
	ErrUserCreateFailed   = apperrors.New("AUTH_USER_CREATE_FAILED", "failed to create authenticated user")
	ErrWorkspaceProvision = apperrors.New("AUTH_WORKSPACE_PROVISION_FAILED", "failed to provision the default workspace")
	ErrSessionCreate      = apperrors.New("AUTH_SESSION_CREATE_FAILED", "failed to create browser session")
	ErrSessionNotFound    = apperrors.New("AUTH_SESSION_NOT_FOUND", "browser session is invalid or expired")
	ErrSessionUnavailable = apperrors.New("AUTH_SESSION_UNAVAILABLE", "browser session service is unavailable")
	ErrSessionResolve     = apperrors.New("AUTH_SESSION_RESOLVE_FAILED", "failed to resolve browser session")
	ErrIdentityConflict   = apperrors.New("AUTH_IDENTITY_CONFLICT", "this email is already registered with a different sign-in method")
)
