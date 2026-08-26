package postgres

import "github.com/box1o/woki/pkg/errors"

var (
	ErrConnection = errors.New("POSTGRES_CONNECTION_FAILED", "PostgreSQL connection failed")
	ErrMigration  = errors.New("POSTGRES_MIGRATION_FAILED", "PostgreSQL migration failed")
	ErrOperation  = errors.New("POSTGRES_OPERATION_FAILED", "PostgreSQL operation failed")
)
