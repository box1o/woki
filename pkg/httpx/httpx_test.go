package httpx

import (
	"net/http/httptest"
	"strings"
	"testing"
)

func TestDecodeJSONRejectsUnknownFields(t *testing.T) {
	r := httptest.NewRequest("POST", "/", strings.NewReader(`{"name":"ok","extra":true}`))
	r.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()
	var dst struct {
		Name string `json:"name"`
	}
	if err := DecodeJSON(w, r, &dst); err == nil {
		t.Fatal("DecodeJSON unexpectedly succeeded")
	}
}

func TestWriteErrorHidesInternalDetails(t *testing.T) {
	w := httptest.NewRecorder()
	WriteError(w, 500, "internal_error", "secret database error")
	if strings.Contains(w.Body.String(), "secret") {
		t.Fatalf("internal detail leaked: %s", w.Body.String())
	}
}
