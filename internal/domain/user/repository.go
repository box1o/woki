package user

import (
	"context"

	"github.com/box1o/woki/pkg/id"
)

type Repository interface {
	Create(context.Context, *User) error
	Update(context.Context, *User) error
	FindByID(context.Context, id.ID) (*User, error)
	FindByEmail(context.Context, string) (*User, error)
	FindByProvider(context.Context, Provider, string) (*User, error)
	Search(context.Context, string, int) ([]*User, error)
}
