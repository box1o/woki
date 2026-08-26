package events

import (
	"context"
	"time"
)

type Type string

type Event interface {
	Type() Type
	OccurredAt() time.Time
	Payload() any
}

type Handler func(context.Context, Event) error

type Bus interface {
	Publish(context.Context, Event) error
	Subscribe(Type, Handler) error
}
