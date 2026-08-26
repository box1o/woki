package health

import (
	"context"
	"net/http"
	"time"

	"github.com/box1o/woki/pkg/httpx"
)

type Checker interface{ Ping(context.Context) error }
type Handler struct{ checks map[string]Checker }

func New(checks map[string]Checker) Handler { return Handler{checks: checks} }
func (h Handler) Register(mux *http.ServeMux) {
	mux.HandleFunc("GET /health", func(w http.ResponseWriter, _ *http.Request) {
		httpx.WriteJSON(w, http.StatusOK, map[string]string{"status": "ok"})
	})
	mux.HandleFunc("GET /ready", h.ready)
}
func (h Handler) ready(w http.ResponseWriter, r *http.Request) {
	status := map[string]string{}
	healthy := true
	for name, checker := range h.checks {
		ctx, cancel := context.WithTimeout(r.Context(), 2*time.Second)
		err := checker.Ping(ctx)
		cancel()
		if err != nil {
			status[name] = "unavailable"
			healthy = false
		} else {
			status[name] = "ok"
		}
	}
	code := http.StatusOK
	if !healthy {
		code = http.StatusServiceUnavailable
	}
	httpx.WriteJSON(w, code, map[string]any{"status": map[bool]string{true: "ready", false: "not_ready"}[healthy], "checks": status})
}
