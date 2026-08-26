package mail

import "github.com/box1o/woki/pkg/errors"

var (
	ErrConfigure = errors.New("MAIL_CONFIGURE_FAILED", "failed to configure email transport")
	ErrSend      = errors.New("MAIL_SEND_FAILED", "failed to send email")
)
