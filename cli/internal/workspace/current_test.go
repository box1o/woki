package workspace

import (
	"context"
	"errors"
	"os"
	"path/filepath"
	"testing"
)

func TestCurrentStorePersistsSelectionPerAPI(t *testing.T) {
	t.Setenv("WOKI_CONFIG_DIR", t.TempDir())
	store, err := NewCurrentStore()
	if err != nil {
		t.Fatal(err)
	}
	ctx := context.Background()
	if err := store.Save(ctx, "https://api.one.example", Current{ID: "one", Name: "alpha"}); err != nil {
		t.Fatal(err)
	}
	if err := store.Save(ctx, "https://api.two.example/", Current{ID: "two", Name: "beta"}); err != nil {
		t.Fatal(err)
	}

	first, err := store.Load(ctx, "https://api.one.example/")
	if err != nil {
		t.Fatal(err)
	}
	if first.ID != "one" || first.Name != "alpha" {
		t.Fatalf("first=%#v", first)
	}
	second, err := store.Load(ctx, "https://api.two.example")
	if err != nil {
		t.Fatal(err)
	}
	if second.ID != "two" || second.Name != "beta" {
		t.Fatalf("second=%#v", second)
	}

	info, err := os.Stat(filepath.Join(os.Getenv("WOKI_CONFIG_DIR"), "config.json"))
	if err != nil {
		t.Fatal(err)
	}
	if got := info.Mode().Perm(); got != 0o600 {
		t.Fatalf("config permissions=%o; want 600", got)
	}
}

func TestCurrentStoreDeleteIsIdempotent(t *testing.T) {
	t.Setenv("WOKI_CONFIG_DIR", t.TempDir())
	store, err := NewCurrentStore()
	if err != nil {
		t.Fatal(err)
	}
	ctx := context.Background()
	if err := store.Delete(ctx, "http://localhost:8080"); err != nil {
		t.Fatal(err)
	}
	if _, err := store.Load(ctx, "http://localhost:8080"); !errors.Is(err, ErrCurrentNotSet) {
		t.Fatalf("Load()=%v; want current-not-set", err)
	}
}
