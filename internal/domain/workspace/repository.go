package workspace

import (
	"context"

	"github.com/box1o/woki/pkg/id"
)

type Repository interface {
	CreateWithOwner(context.Context, *Workspace, *Member) error
	FindByOwnerAndName(context.Context, id.ID, string) (*Workspace, error)
	ListForUser(context.Context, id.ID) ([]*Workspace, error)
	Get(context.Context, id.ID) (*Workspace, error)
	Delete(context.Context, id.ID) error
	FindMember(context.Context, id.ID, id.ID) (*Member, error)
	GetMember(context.Context, id.ID, id.ID) (*Member, error)
	ListMembers(context.Context, id.ID) ([]*Member, error)
	AddMember(context.Context, *Member) error
	RemoveMember(context.Context, id.ID, id.ID) error
	UpdateMemberRole(context.Context, id.ID, id.ID, Role) (*Member, error)
}
