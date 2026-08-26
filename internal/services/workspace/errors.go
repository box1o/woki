package workspace

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrRepositoryRead  = apperrors.New("WORKSPACE_DATABASE_OPERATION_FAILED", "workspace database operation failed")
	ErrRepositoryWrite = apperrors.New("WORKSPACE_DATABASE_OPERATION_FAILED", "workspace database operation failed")
	ErrSearchQuery     = apperrors.New("WORKSPACE_MEMBER_SEARCH_INVALID", "member search query is invalid")
)
