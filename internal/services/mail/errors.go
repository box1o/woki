package mail

import "github.com/box1o/woki/pkg/errors"

var (
	ErrDisabled      = errors.New("MAIL_DISABLED", "mail service is disabled")
	ErrConfiguration = errors.New("MAIL_CONFIGURATION_INVALID", "mail service configuration is invalid")
	ErrTemplate      = errors.New("MAIL_TEMPLATE_FAILED", "failed to render email template")
	ErrSend          = errors.New("MAIL_SEND_FAILED", "failed to send email")
	ErrQueueFull     = errors.New("MAIL_QUEUE_UNAVAILABLE", "mail delivery queue is full")
	ErrShuttingDown  = errors.New("MAIL_QUEUE_UNAVAILABLE", "mail delivery queue is shutting down")
	ErrPayload       = errors.New("MAIL_EVENT_PAYLOAD_INVALID", "mail event payload is invalid")
)
