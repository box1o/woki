package events

import "github.com/box1o/woki/pkg/errors"

var (
	ErrInvalidSubscription = errors.New("EVENT_SUBSCRIPTION_INVALID", "event subscription is invalid")
	ErrHandlerFailed       = errors.New("EVENT_HANDLER_FAILED", "event handler failed")
	ErrHandlerPanic        = errors.New("EVENT_HANDLER_PANIC", "event handler panicked")
)
