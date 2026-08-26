package errors

import (
	"encoding/json"
	stderrors "errors"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func TestStructuredErrorSupportsErrorsIsThroughEnrichment(t *testing.T) {
	base := New("THING_NOT_FOUND", "thing not found")
	wrapped := base.WithErr(stderrors.New("database detail")).WithDetail("thing 42")
	if !stderrors.Is(wrapped, base) {
		t.Fatal("errors.Is did not match by structured code")
	}
}

func TestToHTTPPreservesDomainCodeAndHidesInternalCause(t *testing.T) {
	notFound := New("WORKSPACE_NOT_FOUND", "workspace not found").WithDetail("workspace 42")
	got := ToHTTP(notFound)
	if got.Status != http.StatusNotFound || got.Code != "WORKSPACE_NOT_FOUND" || got.Detail != "workspace 42" {
		t.Fatalf("ToHTTP() = %#v", got)
	}

	internal := Wrap(stderrors.New("secret DSN"), "DATABASE_OPERATION_FAILED", "database operation failed")
	got = ToHTTP(internal)
	if got.Status != http.StatusInternalServerError || got.Message != "Internal server error" || got.Detail != "" {
		t.Fatalf("internal ToHTTP() = %#v", got)
	}
}

func TestWriteErrorUsesEnvelope(t *testing.T) {
	w := httptest.NewRecorder()
	WriteError(w, NewHTTP(http.StatusConflict, "WORKSPACE_NAME_EXISTS", "workspace already exists"))

	var body struct {
		Error HTTPError `json:"error"`
	}
	if err := json.Unmarshal(w.Body.Bytes(), &body); err != nil {
		t.Fatal(err)
	}
	if body.Error.Code != "WORKSPACE_NAME_EXISTS" {
		t.Fatalf("code=%q", body.Error.Code)
	}
}

func TestErrorStringIncludesDetailAndCause(t *testing.T) {
	err := New("WRITE_FAILED", "write failed").
		WithDetail("persist state").
		WithErr(stderrors.New("disk full"))
	got := err.Error()
	if !strings.Contains(got, "persist state") || !strings.Contains(got, "disk full") {
		t.Fatalf("Error()=%q", got)
	}
}

func TestCodeAndDetailHelpers(t *testing.T) {
	err := New("THING_INVALID", "thing invalid").WithDetail("bad thing")
	if !IsCode(err, "thing_invalid") {
		t.Fatalf("Code=%q", Code(err))
	}
	if got := Detail(err); got != "bad thing" {
		t.Fatalf("Detail=%q", got)
	}
}
