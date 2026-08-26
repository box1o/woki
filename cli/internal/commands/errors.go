package commands

import (
	"fmt"

	apperrors "github.com/box1o/woki/pkg/errors"
)

var ErrUsage = apperrors.New("CLI_USAGE_INVALID", "invalid command usage")

func usage(format string, args ...any) error {
	return ErrUsage.WithDetail(fmt.Sprintf(format, args...))
}
