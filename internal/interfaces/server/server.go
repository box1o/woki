// Package server owns HTTP transport composition and process-level HTTP concerns.
package server

import (
	"context"
	"net/http"
	"strings"
	"time"

	"github.com/box1o/woki/internal/interfaces/server/middleware"
	"github.com/box1o/woki/pkg/config"
	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/log"
	contract "github.com/box1o/woki/pkg/ratelimit"
)

type Registrar interface {
	Register(*http.ServeMux)
}

type Server struct {
	addr          string
	allowedOrigin string
	server        *http.Server
}

func New(cfg config.ServerConfig, allowedOrigin string, registrars ...Registrar) *Server {
	return NewWithRateLimiter(cfg, allowedOrigin, nil, config.RateLimitConfig{}, registrars...)
}

func NewWithRateLimiter(cfg config.ServerConfig, allowedOrigin string, limiter contract.Limiter, rate config.RateLimitConfig, registrars ...Registrar) *Server {
	mux := http.NewServeMux()
	for _, registrar := range registrars {
		registrar.Register(mux)
	}

	s := &Server{
		addr:          cfg.Addr,
		allowedOrigin: strings.TrimRight(allowedOrigin, "/"),
	}
	var root http.Handler = mux
	if rate.Enabled {
		root = middleware.RateLimit(limiter, rate.Prefix+":api", contract.Policy{Rate: rate.APILimit, Burst: rate.APIBurst, Period: rate.APIWindow}, false)(root)
	}
	handler := s.securityHeaders(s.requestLog(s.recoverPanic(s.cors(root))))
	s.server = &http.Server{
		Addr:              cfg.Addr,
		Handler:           handler,
		ReadHeaderTimeout: cfg.ReadHeaderTimeout,
		ReadTimeout:       cfg.ReadTimeout,
		WriteTimeout:      cfg.WriteTimeout,
		IdleTimeout:       cfg.IdleTimeout,
		MaxHeaderBytes:    cfg.MaxHeaderBytes,
	}
	return s
}

func (s *Server) Handler() http.Handler { return s.server.Handler }

func (s *Server) Start() error {
	err := s.server.ListenAndServe()
	if err == http.ErrServerClosed {
		return nil
	}
	return err
}

func (s *Server) Shutdown(ctx context.Context) error {
	return s.server.Shutdown(ctx)
}

func (s *Server) cors(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		origin := strings.TrimRight(strings.TrimSpace(r.Header.Get("Origin")), "/")
		if origin != "" {
			w.Header().Add("Vary", "Origin")
		}
		allowed := origin != "" && origin == s.allowedOrigin
		if allowed {
			w.Header().Set("Access-Control-Allow-Origin", origin)
			w.Header().Set("Access-Control-Allow-Credentials", "true")
			w.Header().Set("Access-Control-Allow-Headers", "Authorization, Content-Type")
			w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS")
		}
		if r.Method == http.MethodOptions {
			if origin != "" && !allowed {
				apperrors.WriteError(w, apperrors.ErrForbidden.WithMessage("Origin not allowed").WithDetail("request origin is not allowed"))
				return
			}
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next.ServeHTTP(w, r)
	})
}

func (s *Server) securityHeaders(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Cache-Control", "no-store")
		w.Header().Set("Content-Security-Policy", "default-src 'none'; frame-ancestors 'none'")
		w.Header().Set("Permissions-Policy", "camera=(), geolocation=(), microphone=()")
		w.Header().Set("Referrer-Policy", "no-referrer")
		w.Header().Set("X-Content-Type-Options", "nosniff")
		w.Header().Set("X-Frame-Options", "DENY")
		next.ServeHTTP(w, r)
	})
}

func (s *Server) requestLog(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		started := time.Now()
		rw := &trackingResponseWriter{ResponseWriter: w}
		next.ServeHTTP(rw, r)
		status := rw.status
		if status == 0 {
			status = http.StatusOK
		}
		log.Debug("%s %s -> %d (%s)", r.Method, r.URL.Path, status, time.Since(started).Round(time.Millisecond))
	})
}

func (s *Server) recoverPanic(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		rw := &trackingResponseWriter{ResponseWriter: w}
		defer func() {
			if value := recover(); value != nil {
				log.Error("panic serving %s %s: %v", r.Method, r.URL.Path, value)
				if !rw.wroteHeader {
					apperrors.WriteError(rw, apperrors.ErrInternalServer)
				}
			}
		}()
		next.ServeHTTP(rw, r)
	})
}

type trackingResponseWriter struct {
	http.ResponseWriter
	wroteHeader bool
	status      int
}

func (w *trackingResponseWriter) WriteHeader(status int) {
	if w.wroteHeader {
		return
	}
	w.wroteHeader = true
	w.status = status
	w.ResponseWriter.WriteHeader(status)
}

func (w *trackingResponseWriter) Write(p []byte) (int, error) {
	if !w.wroteHeader {
		w.WriteHeader(http.StatusOK)
	}
	return w.ResponseWriter.Write(p)
}

func (w *trackingResponseWriter) Unwrap() http.ResponseWriter {
	return w.ResponseWriter
}
