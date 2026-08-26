package errors

import (
	stderrors "errors"
	"strings"
	"time"
)

func New(code, message string) *Error {
	return &Error{Code: code, Message: message, Timestamp: time.Now().UTC()}
}

func NewWithDetail(code, message, detail string) *Error {
	return &Error{
		Code:      code,
		Message:   message,
		Details:   detail,
		Timestamp: time.Now().UTC(),
	}
}

func Wrap(err error, code, message string) *Error {
	return &Error{
		Code:      code,
		Message:   message,
		Err:       err,
		Timestamp: time.Now().UTC(),
	}
}

func NewHTTP(status int, code, message string) *HTTPError {
	return &HTTPError{Status: status, Code: code, Message: message}
}

// Code returns the stable application code carried by err, or an empty string.
func Code(err error) string {
	if err == nil {
		return ""
	}
	var appErr *Error
	if stderrors.As(err, &appErr) && appErr != nil {
		return appErr.Code
	}
	var httpErr *HTTPError
	if stderrors.As(err, &httpErr) && httpErr != nil {
		return httpErr.Code
	}
	return ""
}

// IsCode reports whether err carries the provided stable application code.
func IsCode(err error, code string) bool {
	return strings.EqualFold(strings.TrimSpace(Code(err)), strings.TrimSpace(code))
}

// Detail returns the structured detail carried by err, if any.
func Detail(err error) string {
	var appErr *Error
	if stderrors.As(err, &appErr) && appErr != nil {
		return appErr.Details
	}
	var httpErr *HTTPError
	if stderrors.As(err, &httpErr) && httpErr != nil {
		return httpErr.Detail
	}
	return ""
}
