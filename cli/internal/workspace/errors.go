package workspace

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrCurrentNotSet      = apperrors.New("CLI_WORKSPACE_NOT_SELECTED", "no current workspace is selected")
	ErrWorkspaceNotFound  = apperrors.New("CLI_WORKSPACE_NOT_FOUND", "workspace was not found")
	ErrWorkspaceAmbiguous = apperrors.New("CLI_WORKSPACE_AMBIGUOUS", "workspace name is ambiguous")
	ErrMemberNotFound     = apperrors.New("CLI_MEMBER_NOT_FOUND", "workspace member was not found")
	ErrCandidateNotFound  = apperrors.New("CLI_USER_NOT_FOUND", "no matching workspace user was found")
	ErrSelectionInvalid   = apperrors.New("CLI_WORKSPACE_SELECTION_INVALID", "workspace selection is invalid")
	ErrSelectionRead      = apperrors.New("CLI_WORKSPACE_SELECTION_READ_FAILED", "failed to read workspace selection")
	ErrSelectionWrite     = apperrors.New("CLI_WORKSPACE_SELECTION_WRITE_FAILED", "failed to save workspace selection")
)
