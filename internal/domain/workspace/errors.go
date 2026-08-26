package workspace

import "github.com/box1o/woki/pkg/errors"

var (
	ErrNotFound            = errors.New("WORKSPACE_NOT_FOUND", "workspace not found")
	ErrAlreadyExists       = errors.New("WORKSPACE_NAME_EXISTS", "a workspace with the given name already exists")
	ErrMemberNotFound      = errors.New("WORKSPACE_MEMBER_NOT_FOUND", "workspace member not found")
	ErrMemberAlreadyExists = errors.New("WORKSPACE_MEMBER_ALREADY_EXISTS", "workspace member already exists")
	ErrNameEmpty           = errors.New("WORKSPACE_NAME_EMPTY", "workspace name is required")
	ErrNameTooLong         = errors.New("WORKSPACE_NAME_TOO_LONG", "workspace name is too long")
	ErrOwnerRequired       = errors.New("WORKSPACE_OWNER_REQUIRED", "workspace owner is required")
	ErrMemberUserRequired  = errors.New("WORKSPACE_MEMBER_USER_REQUIRED", "workspace member user is required")
	ErrMemberEmailInvalid  = errors.New("WORKSPACE_MEMBER_EMAIL_INVALID", "workspace member email is invalid")
	ErrMemberNameEmpty     = errors.New("WORKSPACE_MEMBER_NAME_EMPTY", "workspace member name is required")
	ErrMemberNameTooLong   = errors.New("WORKSPACE_MEMBER_NAME_TOO_LONG", "workspace member name is too long")
	ErrInvalidRole         = errors.New("WORKSPACE_MEMBER_ROLE_INVALID", "workspace member role is invalid")
	ErrForbidden           = errors.New("WORKSPACE_PERMISSION_DENIED", "workspace operation is forbidden")
	ErrOwnerRemoval        = errors.New("WORKSPACE_OWNER_REMOVAL_INVALID", "workspace owner cannot be removed or demoted")
	ErrDatabaseOperation   = errors.New("WORKSPACE_DATABASE_OPERATION_FAILED", "workspace database operation failed")
)
