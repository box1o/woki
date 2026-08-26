// Package id provides small dependency-free identifiers for domain entities.
package id

import (
	"crypto/rand"
	"fmt"
	"strings"
)

// ID is a UUID-compatible identifier.
type ID string

// New returns a random RFC 4122 version 4 identifier.
func New() ID {
	var b [16]byte
	if _, err := rand.Read(b[:]); err != nil {
		panic(fmt.Sprintf("id: crypto/rand failed: %v", err))
	}
	b[6] = (b[6] & 0x0f) | 0x40
	b[8] = (b[8] & 0x3f) | 0x80
	return ID(fmt.Sprintf("%08x-%04x-%04x-%04x-%012x",
		b[0:4], b[4:6], b[6:8], b[8:10], b[10:16]))
}

// Parse validates s and returns its normalized identifier form.
func Parse(s string) (ID, error) {
	s = strings.ToLower(strings.TrimSpace(s))
	if len(s) != 36 || s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-' {
		return "", ErrInvalid.WithDetail(s)
	}
	for i, c := range s {
		if i == 8 || i == 13 || i == 18 || i == 23 {
			continue
		}
		if !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) {
			return "", ErrInvalid.WithDetail(s)
		}
	}
	return ID(s), nil
}

// String returns the textual representation of id.
func (id ID) String() string { return string(id) }

// IsZero reports whether id is empty.
func (id ID) IsZero() bool { return id == "" }

// Valid reports whether id has the canonical UUID-compatible form.
func (id ID) Valid() bool { _, err := Parse(id.String()); return err == nil }
