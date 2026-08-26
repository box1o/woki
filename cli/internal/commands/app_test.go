package commands

import (
	"bytes"
	"context"
	"strings"
	"testing"
)

func TestRunHelp(t *testing.T) {
	var out, errOut bytes.Buffer
	code := New(&out, &errOut).Run(context.Background(), []string{"help"})
	if code != 0 {
		t.Fatalf("exit code=%d, stderr=%q", code, errOut.String())
	}
	if !strings.Contains(out.String(), "Woki CLI") {
		t.Fatalf("help output=%q", out.String())
	}
}

func TestRunVersion(t *testing.T) {
	var out, errOut bytes.Buffer
	code := New(&out, &errOut).Run(context.Background(), []string{"version"})
	if code != 0 || strings.TrimSpace(out.String()) != "woki dev" {
		t.Fatalf("code=%d stdout=%q stderr=%q", code, out.String(), errOut.String())
	}
}

func TestFailExitCodes(t *testing.T) {
	var out, errOut bytes.Buffer
	app := New(&out, &errOut)
	if got := app.fail(context.Canceled); got != 130 {
		t.Fatalf("context cancellation exit=%d; want 130", got)
	}
	if got := app.fail(usage("bad usage")); got != 2 {
		t.Fatalf("usage exit=%d; want 2", got)
	}
}
