package id

import "testing"

func TestNewProducesValidDistinctIDs(t *testing.T) {
	a, b := New(), New()
	if a == b {
		t.Fatal("duplicate IDs")
	}
	if !a.Valid() || !b.Valid() {
		t.Fatalf("invalid IDs: %q %q", a, b)
	}
	if len(a.String()) != 36 {
		t.Fatalf("ID length=%d", len(a.String()))
	}
}
