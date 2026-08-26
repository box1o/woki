package mail

import "github.com/box1o/woki/pkg/errors"

var ErrUnavailable = errors.NewHTTP(503, "MAIL_UNAVAILABLE", "Mail service is unavailable")
