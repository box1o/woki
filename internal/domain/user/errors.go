package user

import "github.com/box1o/woki/pkg/errors"

var (
	ErrNotFound          = errors.New("USER_NOT_FOUND", "user not found")
	ErrAlreadyExists     = errors.New("USER_ALREADY_EXISTS", "user already exists")
	ErrEmailInvalid      = errors.New("USER_EMAIL_INVALID", "user email is invalid")
	ErrEmailTooLong      = errors.New("USER_EMAIL_TOO_LONG", "user email is too long")
	ErrNameEmpty         = errors.New("USER_NAME_EMPTY", "user name is required")
	ErrNameTooLong       = errors.New("USER_NAME_TOO_LONG", "user name is too long")
	ErrProviderEmpty     = errors.New("USER_PROVIDER_REQUIRED", "user provider is required")
	ErrProviderInvalid   = errors.New("USER_PROVIDER_INVALID", "user provider is invalid")
	ErrProviderIDEmpty   = errors.New("USER_PROVIDER_ID_REQUIRED", "user provider ID is required")
	ErrProviderIDTooLong = errors.New("USER_PROVIDER_ID_TOO_LONG", "user provider ID is too long")
	ErrDatabaseOperation = errors.New("USER_DATABASE_OPERATION_FAILED", "user database operation failed")
)
