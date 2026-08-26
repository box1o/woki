package config

import "github.com/box1o/woki/pkg/errors"

var (
	ErrInvalid      = errors.New("CONFIG_INVALID", "configuration is invalid")
	ErrEnvFile      = errors.New("CONFIG_ENV_FILE_FAILED", "failed to load environment file")
	ErrValueParse   = errors.New("CONFIG_VALUE_INVALID", "configuration value is invalid")
	ErrLogConfig    = errors.New("CONFIG_LOG_INVALID", "logging configuration is invalid")
	ErrAuthRequired = errors.New("CONFIG_AUTH_REQUIRED", "at least one authentication provider must be configured")
)
