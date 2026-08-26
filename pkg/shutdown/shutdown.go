// Package shutdown coordinates graceful process termination.
package shutdown

import (
	"context"
	"errors"
	"os"
	"os/signal"
	"syscall"
	"time"
)

type Stopper interface{ Shutdown(context.Context) error }

func Wait(stopper Stopper, timeout time.Duration) error {
	if stopper == nil {
		return ErrFailed.WithDetail("shutdown target is nil")
	}
	if timeout <= 0 {
		return ErrInvalidTimeout
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	<-ctx.Done()
	shutdownCtx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()
	if err := stopper.Shutdown(shutdownCtx); err != nil && !errors.Is(err, context.Canceled) {
		return ErrFailed.WithErr(err)
	}
	return nil
}
