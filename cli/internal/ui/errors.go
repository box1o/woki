package ui

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrTableInvalid     = apperrors.New("CLI_TABLE_INVALID", "CLI table data is invalid")
	ErrSelectionEmpty   = apperrors.New("CLI_SELECTION_EMPTY", "there are no options to select")
	ErrSelectionInvalid = apperrors.New("CLI_SELECTION_INVALID", "selection is invalid")
)
