package credentials

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrNotFound       = apperrors.New("CLI_LOCAL_CREDENTIAL_NOT_FOUND", "local CLI credential was not found")
	ErrInvalid        = apperrors.New("CLI_LOCAL_CREDENTIAL_INVALID", "local CLI credential is invalid or expired")
	ErrConfigDir      = apperrors.New("CLI_CONFIG_DIRECTORY_FAILED", "failed to resolve CLI configuration directory")
	ErrRead           = apperrors.New("CLI_CREDENTIAL_READ_FAILED", "failed to read local CLI credential")
	ErrWrite          = apperrors.New("CLI_CREDENTIAL_WRITE_FAILED", "failed to persist local CLI credential")
	ErrDelete         = apperrors.New("CLI_CREDENTIAL_DELETE_FAILED", "failed to delete local CLI credential")
	ErrDecode         = apperrors.New("CLI_CREDENTIAL_DECODE_FAILED", "failed to decode local CLI credential")
	ErrKeyring        = apperrors.New("CLI_KEYRING_FAILED", "system credential store operation failed")
	ErrFileTooLarge   = apperrors.New("CLI_CREDENTIAL_FILE_TOO_LARGE", "local CLI credential file is unexpectedly large")
	ErrFilePermission = apperrors.New("CLI_CREDENTIAL_PERMISSION_FAILED", "failed to secure local CLI credential")
)
