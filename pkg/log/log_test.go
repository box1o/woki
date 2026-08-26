package log

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestParseLevelAndOutput(t *testing.T) {
	if got, err := ParseLevel("DEBUG"); err != nil || got != DebugLevel {
		t.Fatalf("ParseLevel()=(%v,%v)", got, err)
	}
	if got, err := ParseOutput("both"); err != nil || got != ConsoleAndFile {
		t.Fatalf("ParseOutput()=(%v,%v)", got, err)
	}
	if _, err := ParseLevel("verbose"); err == nil {
		t.Fatal("ParseLevel(verbose) succeeded")
	}
}

func TestFileLoggingUsesPrivatePermissions(t *testing.T) {
	path := filepath.Join(t.TempDir(), "nested", "woki.log")
	if err := Configure(Config{Output: File, Level: DebugLevel, FilePath: path, DisableColors: true}); err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = Cleanup() })
	Debug("device %s", "ready")

	info, err := os.Stat(path)
	if err != nil {
		t.Fatal(err)
	}
	if got := info.Mode().Perm(); got != 0o600 {
		t.Fatalf("mode=%#o; want 0600", got)
	}
	body, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(string(body), "device ready") {
		t.Fatalf("log=%q", body)
	}
}

func TestOutputTypeValid(t *testing.T) {
	if !Console.Valid() || !File.Valid() || !ConsoleAndFile.Valid() {
		t.Fatal("known output types should be valid")
	}
	if OutputType(99).Valid() {
		t.Fatal("unknown output type should be invalid")
	}
}
