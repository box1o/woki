package errors

import (
	"encoding/json"
	stderrors "errors"
	"net/http"
	"strings"
)

const (
	CodeBadRequest         = "BAD_REQUEST"
	CodeUnauthorized       = "UNAUTHORIZED"
	CodeForbidden          = "FORBIDDEN"
	CodeNotFound           = "NOT_FOUND"
	CodeConflict           = "CONFLICT"
	CodeValidation         = "VALIDATION_ERROR"
	CodeInternalError      = "INTERNAL_ERROR"
	CodeNotImplemented     = "NOT_IMPLEMENTED"
	CodeServiceUnavailable = "SERVICE_UNAVAILABLE"
)

var (
	ErrBadRequest         = NewHTTP(http.StatusBadRequest, CodeBadRequest, "Invalid request")
	ErrUnauthorized       = NewHTTP(http.StatusUnauthorized, CodeUnauthorized, "Authentication required")
	ErrForbidden          = NewHTTP(http.StatusForbidden, CodeForbidden, "Access denied")
	ErrNotFound           = NewHTTP(http.StatusNotFound, CodeNotFound, "Resource not found")
	ErrConflict           = NewHTTP(http.StatusConflict, CodeConflict, "Resource conflict")
	ErrValidation         = NewHTTP(http.StatusBadRequest, CodeValidation, "Validation failed")
	ErrInternalServer     = NewHTTP(http.StatusInternalServerError, CodeInternalError, "Internal server error")
	ErrNotImplemented     = NewHTTP(http.StatusNotImplemented, CodeNotImplemented, "Feature not implemented")
	ErrServiceUnavailable = NewHTTP(http.StatusServiceUnavailable, CodeServiceUnavailable, "Service temporarily unavailable")
	ErrNameExists         = NewHTTP(http.StatusConflict, "NAME_EXISTS", "Resource with the given name already exists")
)

type responseEnvelope struct {
	Error *HTTPError `json:"error"`
}

// WriteError writes a structured error in Woki's public API envelope.
func WriteError(w http.ResponseWriter, err *HTTPError) {
	if err == nil {
		err = ErrInternalServer
	}
	if err.Status >= http.StatusInternalServerError {
		// Internal implementation details must never cross the HTTP boundary.
		err = &HTTPError{Status: err.Status, Code: err.Code, Message: err.Message}
	}
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(err.Status)
	_ = json.NewEncoder(w).Encode(responseEnvelope{Error: err})
}

// Write converts any structured application error to HTTP and writes it.
func Write(w http.ResponseWriter, err error) {
	WriteError(w, ToHTTP(err))
}

// ToHTTP maps application/domain errors to transport errors while preserving
// their stable code and safe message. Wrapped errors are resolved with
// errors.As, so service-level enrichment does not break transport mapping.
func ToHTTP(err error) *HTTPError {
	if err == nil {
		return ErrInternalServer
	}

	var httpErr *HTTPError
	if stderrors.As(err, &httpErr) && httpErr != nil {
		return httpErr
	}

	var domainErr *Error
	if !stderrors.As(err, &domainErr) || domainErr == nil {
		return ErrInternalServer
	}

	status := statusForCode(domainErr.Code)
	message := domainErr.Message
	if status >= http.StatusInternalServerError {
		message = ErrInternalServer.Message
	}
	mapped := NewHTTP(status, domainErr.Code, message)
	if status < http.StatusInternalServerError && domainErr.Details != "" {
		mapped = mapped.WithDetail(domainErr.Details)
	}
	return mapped
}

func statusForCode(code string) int {
	code = strings.ToUpper(strings.TrimSpace(code))

	switch {
	case strings.Contains(code, "CREDENTIAL_NOT_FOUND"),
		strings.Contains(code, "CREDENTIAL_EXPIRED"),
		strings.Contains(code, "CREDENTIAL_REQUIRED"),
		strings.Contains(code, "SESSION_NOT_FOUND"),
		strings.Contains(code, "SESSION_REQUIRED"),
		strings.Contains(code, "UNAUTHORIZED"):
		return http.StatusUnauthorized
	case strings.Contains(code, "PERMISSION_DENIED"),
		strings.Contains(code, "FORBIDDEN"),
		strings.Contains(code, "AUTHORIZATION_DENIED"):
		return http.StatusForbidden
	case strings.Contains(code, "NOT_FOUND"):
		return http.StatusNotFound
	case strings.Contains(code, "AUTHORIZATION_EXPIRED"):
		return http.StatusGone
	case strings.Contains(code, "EXISTS"),
		strings.Contains(code, "CONFLICT"),
		strings.Contains(code, "PENDING"),
		strings.Contains(code, "ALREADY_HANDLED"):
		return http.StatusConflict
	case strings.Contains(code, "RATE_LIMIT"):
		return http.StatusTooManyRequests
	case strings.Contains(code, "CAPACITY"),
		strings.Contains(code, "UNAVAILABLE"):
		return http.StatusServiceUnavailable
	case strings.Contains(code, "INVALID"),
		strings.Contains(code, "REQUIRED"),
		strings.Contains(code, "EMPTY"),
		strings.Contains(code, "TOO_LONG"):
		return http.StatusBadRequest
	case strings.Contains(code, "NOT_IMPLEMENTED"):
		return http.StatusNotImplemented
	case strings.Contains(code, "FAILED"),
		strings.Contains(code, "INTERNAL"),
		strings.Contains(code, "DATABASE"):
		return http.StatusInternalServerError
	default:
		return http.StatusInternalServerError
	}
}
