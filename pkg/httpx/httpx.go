// Package httpx contains small HTTP transport helpers.
package httpx

import (
	"encoding/json"
	stderrors "errors"
	"fmt"
	"io"
	"mime"
	"net/http"
	"strings"

	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/log"
)

const maxBodyBytes int64 = 1 << 20

var (
	ErrInvalidContentType = apperrors.New("HTTP_CONTENT_TYPE_INVALID", "Content type must be application/json")
	ErrInvalidJSON        = apperrors.New("HTTP_JSON_INVALID", "Invalid JSON body")
)

func DecodeJSON(w http.ResponseWriter, r *http.Request, dst any) error {
	contentType := strings.TrimSpace(r.Header.Get("Content-Type"))
	if contentType == "" {
		return ErrInvalidContentType
	}
	mediaType, _, err := mime.ParseMediaType(contentType)
	if err != nil || (mediaType != "application/json" && !strings.HasSuffix(mediaType, "+json")) {
		return ErrInvalidContentType
	}

	r.Body = http.MaxBytesReader(w, r.Body, maxBodyBytes)
	dec := json.NewDecoder(r.Body)
	dec.DisallowUnknownFields()
	if err := dec.Decode(dst); err != nil {
		return ErrInvalidJSON.WithErr(err)
	}
	if err := dec.Decode(&struct{}{}); !stderrors.Is(err, io.EOF) {
		return ErrInvalidJSON.WithDetail("body must contain exactly one JSON value")
	}
	return nil
}

func WriteJSON(w http.ResponseWriter, status int, value any) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	if value != nil {
		_ = json.NewEncoder(w).Encode(value)
	}
}

// WriteError is kept as a compact compatibility helper for handlers that need
// to define a transport-specific code directly.
func WriteError(w http.ResponseWriter, status int, code, message string) {
	if status >= http.StatusInternalServerError {
		message = apperrors.ErrInternalServer.Message
	}
	apperrors.WriteError(w, apperrors.NewHTTP(status, strings.ToUpper(strings.TrimSpace(code)), message))
}

// WriteMappedError maps a structured domain/service error through pkg/errors.
func WriteMappedError(w http.ResponseWriter, err error) {
	if err == nil {
		return
	}
	mapped := apperrors.ToHTTP(err)
	if mapped.Status >= http.StatusInternalServerError {
		log.Error("HTTP request failed [%s]: %v", apperrors.Code(err), err)
	}
	apperrors.WriteError(w, mapped)
}

// WrapDecodeError converts a decode failure into a stable bad-request error.
func WrapDecodeError(err error, message string) *apperrors.HTTPError {
	if err == nil {
		return nil
	}
	return apperrors.NewHTTP(http.StatusBadRequest, "INVALID_REQUEST", message)
}

// Internal wraps an internal cause without exposing it through the HTTP layer.
func Internal(err error, message string) error {
	if err == nil {
		return nil
	}
	return apperrors.Wrap(err, "HTTP_INTERNAL_FAILED", fmt.Sprintf("%s", message))
}
