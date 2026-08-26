package shutdown

import (
	"context"
	"errors"
	"testing"
)

type failingStopper struct{ err error }

func (s failingStopper) Shutdown(context.Context) error { return s.err }

func TestWaitRejectsInvalidInputs(t *testing.T) {
	if err := Wait(nil, 1); !errors.Is(err, ErrFailed) {
		t.Fatalf("Wait(nil)=%v; want ErrFailed", err)
	}
	if err := Wait(failingStopper{}, 0); !errors.Is(err, ErrInvalidTimeout) {
		t.Fatalf("Wait(timeout=0)=%v; want ErrInvalidTimeout", err)
	}
}
