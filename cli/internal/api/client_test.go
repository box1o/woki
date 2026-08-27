package api

import (
	"errors"
	"io"
	"net/http"
	"strings"
	"testing"
)

func TestWithTokenDoesNotMutateClient(t *testing.T) {
	c, err := New("http://localhost:3000")
	if err != nil {
		t.Fatal(err)
	}
	authed := c.WithToken("secret")
	if c.token != "" {
		t.Fatal("WithToken mutated base client")
	}
	if authed.token != "secret" {
		t.Fatalf("token=%q", authed.token)
	}
	if authed == c {
		t.Fatal("WithToken returned same pointer")
	}
}

func TestDecodeErrorPreservesSafeDetail(t *testing.T) {
	resp := &http.Response{
		StatusCode: http.StatusBadRequest,
		Status:     "400 Bad Request",
		Body:       io.NopCloser(strings.NewReader(`{"error":{"code":"WORKSPACE_INVALID","message":"invalid workspace","detail":"name is required"}}`)),
	}
	err := decodeError(resp)
	var apiErr *Error
	if !errors.As(err, &apiErr) {
		t.Fatalf("decodeError()=%T %v; want *Error", err, err)
	}
	if apiErr.Code != "WORKSPACE_INVALID" || apiErr.Detail != "name is required" {
		t.Fatalf("unexpected error: %+v", apiErr)
	}
}
