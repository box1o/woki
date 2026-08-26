package cli

import "github.com/box1o/woki/pkg/errors"

var (
	ErrCredentialNotFound      = errors.New("CLI_CREDENTIAL_NOT_FOUND", "CLI credential was not found")
	ErrCredentialAlreadyExists = errors.New("CLI_CREDENTIAL_CONFLICT", "CLI credential already exists")
	ErrCredentialExpired       = errors.New("CLI_CREDENTIAL_EXPIRED", "CLI credential has expired")
	ErrClientNameEmpty         = errors.New("CLI_NAME_REQUIRED", "CLI credential name is required")
	ErrClientNameTooLong       = errors.New("CLI_NAME_TOO_LONG", "CLI credential name is too long")
	ErrOwnerRequired           = errors.New("CLI_OWNER_REQUIRED", "CLI credential owner is required")
	ErrTokenHashInvalid        = errors.New("CLI_TOKEN_HASH_INVALID", "CLI credential token hash is invalid")
	ErrInvalidExpiry           = errors.New("CLI_EXPIRY_INVALID", "CLI credential expiry is invalid")
	ErrDatabaseOperation       = errors.New("CLI_DATABASE_OPERATION_FAILED", "CLI credential database operation failed")
)
