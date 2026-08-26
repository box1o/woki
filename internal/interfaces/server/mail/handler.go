package mail

import (
	"context"
	"net/http"
	"strings"

	"github.com/box1o/woki/internal/interfaces/server/middleware"
	mailservice "github.com/box1o/woki/internal/services/mail"
	"github.com/box1o/woki/pkg/config"
	"github.com/box1o/woki/pkg/httpx"
	contract "github.com/box1o/woki/pkg/ratelimit"
)

type IssueSender interface {
	SendIssue(context.Context, string, string, string) error
}
type Handler struct {
	service IssueSender
	auth    *middleware.Authenticator
	limiter contract.Limiter
	rate    config.RateLimitConfig
}

func New(service IssueSender, auth *middleware.Authenticator) *Handler {
	return &Handler{service: service, auth: auth}
}
func NewWithLimiter(service IssueSender, auth *middleware.Authenticator, limiter contract.Limiter, rate config.RateLimitConfig) *Handler {
	return &Handler{service: service, auth: auth, limiter: limiter, rate: rate}
}
func (h *Handler) Register(mux *http.ServeMux) {
	var handler http.Handler = http.HandlerFunc(h.issue)
	if h.rate.Enabled {
		handler = middleware.RateLimit(h.limiter, h.rate.Prefix+":mail", contract.Policy{Rate: h.rate.MailLimit, Burst: h.rate.MailBurst, Period: h.rate.MailWindow}, true)(handler)
	}
	handler = h.auth.RequireWeb(handler)
	mux.Handle("POST /mail/issue", handler)
}
func (h *Handler) issue(w http.ResponseWriter, r *http.Request) {
	principal, ok := middleware.PrincipalFrom(r.Context())
	if !ok || principal.User == nil {
		httpx.WriteMappedError(w, ErrUnavailable)
		return
	}
	var req struct {
		Subject string `json:"subject"`
		Body    string `json:"body"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	if err := h.service.SendIssue(r.Context(), principal.User.Email, strings.TrimSpace(req.Subject), strings.TrimSpace(req.Body)); err != nil {
		if err == mailservice.ErrDisabled {
			httpx.WriteMappedError(w, ErrUnavailable)
			return
		}
		httpx.WriteMappedError(w, err)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}
