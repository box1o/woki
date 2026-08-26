// Package errors defines structured application, domain, and HTTP errors.
package errors

import (
	"fmt"
	"time"
)

// Error is the common structured error used by domain and service packages.
// Err may retain the underlying implementation error for errors.Is/errors.As
// and server-side diagnostics, but it is never serialized to clients.
type Error struct {
	Code      string    `json:"code"`
	Message   string    `json:"message"`
	Details   string    `json:"details,omitempty"`
	Err       error     `json:"-"`
	Timestamp time.Time `json:"timestamp"`
}

func (e *Error) Error() string {
	if e == nil {
		return "<nil>"
	}
	switch {
	case e.Details != "" && e.Err != nil:
		return fmt.Sprintf("%s: %s: %v", e.Message, e.Details, e.Err)
	case e.Details != "":
		return fmt.Sprintf("%s: %s", e.Message, e.Details)
	case e.Err != nil:
		return fmt.Sprintf("%s: %v", e.Message, e.Err)
	default:
		return e.Message
	}
}

func (e *Error) Unwrap() error {
	if e == nil {
		return nil
	}
	return e.Err
}

// Is compares structured errors by stable code. This keeps errors.Is useful
// even when an error has been enriched with details or an underlying cause.
func (e *Error) Is(target error) bool {
	if e == nil {
		return target == nil
	}
	t, ok := target.(*Error)
	return ok && t != nil && e.Code == t.Code
}

func (e *Error) WithDetail(detail string) *Error {
	if e == nil {
		return nil
	}
	return &Error{
		Code:      e.Code,
		Message:   e.Message,
		Details:   detail,
		Err:       e.Err,
		Timestamp: time.Now().UTC(),
	}
}

func (e *Error) WithErr(err error) *Error {
	if e == nil {
		return nil
	}
	return &Error{
		Code:      e.Code,
		Message:   e.Message,
		Details:   e.Details,
		Err:       err,
		Timestamp: time.Now().UTC(),
	}
}

func (e *Error) WithMessage(message string) *Error {
	if e == nil {
		return nil
	}
	return &Error{
		Code:      e.Code,
		Message:   message,
		Details:   e.Details,
		Err:       e.Err,
		Timestamp: time.Now().UTC(),
	}
}

// DomainError is retained as a semantic alias for domain packages.
type DomainError = Error

// HTTPError is the transport-safe representation of an application error.
type HTTPError struct {
	Status  int    `json:"-"`
	Code    string `json:"code"`
	Message string `json:"message"`
	Detail  string `json:"detail,omitempty"`
}

func (e *HTTPError) Error() string {
	if e == nil {
		return "<nil>"
	}
	if e.Detail != "" {
		return fmt.Sprintf("%s: %s", e.Message, e.Detail)
	}
	return e.Message
}

func (e *HTTPError) Is(target error) bool {
	if e == nil {
		return target == nil
	}
	t, ok := target.(*HTTPError)
	return ok && t != nil && e.Code == t.Code
}

func (e *HTTPError) WithMessage(message string) *HTTPError {
	if e == nil {
		return nil
	}
	return &HTTPError{Status: e.Status, Code: e.Code, Message: message, Detail: e.Detail}
}

func (e *HTTPError) WithDetail(detail string) *HTTPError {
	if e == nil {
		return nil
	}
	return &HTTPError{Status: e.Status, Code: e.Code, Message: e.Message, Detail: detail}
}
