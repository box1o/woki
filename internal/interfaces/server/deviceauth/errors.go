package deviceauth

import apperrors "github.com/box1o/woki/pkg/errors"

var ErrRateLimited = apperrors.New("DEVICE_AUTH_RATE_LIMITED", "too many device authorization requests")
