package auth

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrCredentialSave       = apperrors.New("CLI_CREDENTIAL_SAVE_FAILED", "failed to save CLI credential")
	ErrCredentialRevoke     = apperrors.New("CLI_CREDENTIAL_REVOKE_FAILED", "failed to revoke CLI credential")
	ErrAuthorizationExpired = apperrors.New("DEVICE_AUTHORIZATION_EXPIRED", "device authorization expired")
)
