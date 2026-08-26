package deviceauth

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrStateStorage         = apperrors.New("DEVICE_AUTH_STATE_FAILED", "device authorization state operation failed")
	ErrInvalidCode          = apperrors.New("DEVICE_AUTHORIZATION_INVALID", "invalid device authorization code")
	ErrAuthorizationExpired = apperrors.New("DEVICE_AUTHORIZATION_EXPIRED", "device authorization expired")
	ErrAuthorizationPending = apperrors.New("DEVICE_AUTHORIZATION_PENDING", "device authorization is still pending")
	ErrAuthorizationDenied  = apperrors.New("DEVICE_AUTHORIZATION_DENIED", "device authorization was denied")
	ErrAlreadyHandled       = apperrors.New("DEVICE_AUTHORIZATION_ALREADY_HANDLED", "device authorization has already been handled")
	ErrCapacityReached      = apperrors.New("DEVICE_AUTHORIZATION_CAPACITY_REACHED", "device authorization is temporarily unavailable")
	ErrCodeGenerationFailed = apperrors.New("DEVICE_AUTHORIZATION_CODE_GENERATION_FAILED", "failed to generate device authorization code")
	ErrCredentialCreate     = apperrors.New("CLI_CREDENTIAL_CREATE_FAILED", "failed to create CLI credential")
	ErrTokenGeneration      = apperrors.New("CLI_TOKEN_GENERATION_FAILED", "failed to generate CLI access token")
	ErrOwnerRequired        = apperrors.New("DEVICE_AUTHORIZATION_OWNER_REQUIRED", "authenticated owner is required")
)
