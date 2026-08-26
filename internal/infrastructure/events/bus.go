package events

import (
	"context"
	stderrors "errors"
	"fmt"
	"sync"

	domain "github.com/box1o/woki/internal/domain/events"
)

type Bus struct {
	mu       sync.RWMutex
	handlers map[domain.Type][]domain.Handler
}

func NewBus() *Bus { return &Bus{handlers: make(map[domain.Type][]domain.Handler)} }
func (b *Bus) Subscribe(t domain.Type, handler domain.Handler) error {
	if t == "" || handler == nil {
		return ErrInvalidSubscription
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	b.handlers[t] = append(b.handlers[t], handler)
	return nil
}
func (b *Bus) Publish(ctx context.Context, event domain.Event) error {
	if event == nil {
		return ErrHandlerFailed.WithDetail("event is nil")
	}
	b.mu.RLock()
	handlers := append([]domain.Handler(nil), b.handlers[event.Type()]...)
	b.mu.RUnlock()
	var errs []error
	for _, handler := range handlers {
		if err := callHandler(ctx, handler, event); err != nil {
			errs = append(errs, ErrHandlerFailed.WithDetail(string(event.Type())).WithErr(err))
		}
	}
	if len(errs) > 0 {
		return ErrHandlerFailed.WithErr(stderrors.Join(errs...))
	}
	return nil
}

func callHandler(ctx context.Context, handler domain.Handler, event domain.Event) (err error) {
	defer func() {
		if recovered := recover(); recovered != nil {
			err = ErrHandlerPanic.WithErr(fmt.Errorf("%v", recovered))
		}
	}()
	return handler(ctx, event)
}
