package platform

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrBrowserUnsupported = apperrors.New("CLI_BROWSER_UNSUPPORTED", "opening a browser is unsupported on this platform")
	ErrBrowserOpen        = apperrors.New("CLI_BROWSER_OPEN_FAILED", "failed to open browser")
)
