package cli

import (
	"context"

	"github.com/box1o/woki/pkg/id"
)

type Repository interface {
	Create(context.Context, *Credential) error
	FindByID(context.Context, id.ID) (*Credential, error)
	FindByTokenHash(context.Context, string) (*Credential, error)
	Delete(context.Context, id.ID) error
}
