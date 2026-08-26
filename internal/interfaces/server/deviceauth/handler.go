package deviceauth

import (
	"context"
	"crypto/sha256"
	"encoding/hex"
	stderrors "errors"
	"net/http"
	"strconv"
	"strings"
	"time"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/interfaces/server/middleware"
	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/config"
	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/httpx"
	"github.com/box1o/woki/pkg/id"
	contract "github.com/box1o/woki/pkg/ratelimit"
)

type Service interface {
	Create(context.Context, string) (api.DeviceCodeResponse, error)
	Inspect(context.Context, string) (api.DeviceRequest, error)
	Approve(context.Context, string, id.ID) error
	Deny(context.Context, string, id.ID) error
	Exchange(context.Context, string) (api.DeviceTokenResponse, error)
	Revoke(context.Context, id.ID) error
}

type Handler struct {
	service Service
	auth    *middleware.Authenticator
	limiter contract.Limiter
	rate    config.RateLimitConfig
}

func New(service Service, auth *middleware.Authenticator) *Handler {
	return &Handler{service: service, auth: auth}
}
func NewWithLimiter(service Service, auth *middleware.Authenticator, limiter contract.Limiter, rate config.RateLimitConfig) *Handler {
	return &Handler{service: service, auth: auth, limiter: limiter, rate: rate}
}

func (h *Handler) Register(mux *http.ServeMux) {
	mux.HandleFunc("POST /auth/device/code", h.create)
	mux.Handle("GET /auth/device/request", h.auth.RequireWeb(http.HandlerFunc(h.inspect)))
	mux.Handle("POST /auth/device/approve", h.auth.RequireWeb(http.HandlerFunc(h.approve)))
	mux.Handle("POST /auth/device/deny", h.auth.RequireWeb(http.HandlerFunc(h.deny)))
	mux.HandleFunc("POST /auth/device/token", h.exchange)
	mux.Handle("GET /auth/cli/status", h.auth.RequireCLI(http.HandlerFunc(h.cliStatus)))
	mux.Handle("POST /auth/cli/logout", h.auth.RequireCLI(http.HandlerFunc(h.cliLogout)))
}

func (h *Handler) create(w http.ResponseWriter, r *http.Request) {
	if !h.allow(w, r, "device-code", h.rate.Prefix+":device-code:ip:"+middleware.ClientIP(r), contract.Policy{Rate: h.rate.DeviceCodeLimit, Burst: h.rate.DeviceCodeBurst, Period: h.rate.DeviceCodeWindow}) {
		return
	}
	var req struct {
		ClientName string `json:"client_name"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	response, err := h.service.Create(r.Context(), req.ClientName)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	httpx.WriteJSON(w, http.StatusOK, response)
}
func (h *Handler) inspect(w http.ResponseWriter, r *http.Request) {
	response, err := h.service.Inspect(r.Context(), strings.TrimSpace(r.URL.Query().Get("code")))
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	httpx.WriteJSON(w, http.StatusOK, response)
}
func (h *Handler) approve(w http.ResponseWriter, r *http.Request) {
	var req struct {
		UserCode string `json:"user_code"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	principal, ok := middleware.PrincipalFrom(r.Context())
	if !ok || principal.User == nil {
		apperrors.WriteError(w, apperrors.ErrUnauthorized)
		return
	}
	if err := h.service.Approve(r.Context(), req.UserCode, principal.User.ID); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}
func (h *Handler) deny(w http.ResponseWriter, r *http.Request) {
	var req struct {
		UserCode string `json:"user_code"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	principal, ok := middleware.PrincipalFrom(r.Context())
	if !ok || principal.User == nil {
		apperrors.WriteError(w, apperrors.ErrUnauthorized)
		return
	}
	if err := h.service.Deny(r.Context(), req.UserCode, principal.User.ID); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}
func (h *Handler) exchange(w http.ResponseWriter, r *http.Request) {
	var req struct {
		DeviceCode string `json:"device_code"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	key := h.rate.Prefix + ":device-poll:" + devicePollKey(req.DeviceCode)
	if !h.allow(w, r, "device-poll", key, contract.Policy{Rate: h.rate.DevicePollLimit, Burst: h.rate.DevicePollBurst, Period: h.rate.DevicePollWindow}) {
		return
	}
	response, err := h.service.Exchange(r.Context(), req.DeviceCode)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	httpx.WriteJSON(w, http.StatusOK, response)
}
func (h *Handler) cliStatus(w http.ResponseWriter, r *http.Request) {
	principal, ok := middleware.PrincipalFrom(r.Context())
	if !ok || principal.User == nil || principal.Credential == nil {
		apperrors.WriteError(w, apperrors.ErrUnauthorized.WithMessage("Valid CLI credential required"))
		return
	}
	httpx.WriteJSON(w, http.StatusOK, map[string]any{"authenticated": true, "user": api.User{ID: principal.User.ID.String(), Email: principal.User.Email, Name: principal.User.Name, AvatarURL: principal.User.AvatarURL}, "client_name": principal.Credential.ClientName, "expires_at": principal.Credential.ExpiresAt})
}
func (h *Handler) cliLogout(w http.ResponseWriter, r *http.Request) {
	principal, ok := middleware.PrincipalFrom(r.Context())
	if !ok || principal.Credential == nil {
		apperrors.WriteError(w, apperrors.ErrUnauthorized.WithMessage("Valid CLI credential required"))
		return
	}
	if err := h.service.Revoke(r.Context(), principal.Credential.ID); err != nil && !stderrors.Is(err, domaincli.ErrCredentialNotFound) {
		httpx.WriteMappedError(w, err)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}

func (h *Handler) allow(w http.ResponseWriter, r *http.Request, namespace, key string, policy contract.Policy) bool {
	if h.limiter == nil || !h.rate.Enabled || !policy.Valid() {
		return true
	}
	decision, err := h.limiter.Allow(r.Context(), key, policy)
	if err != nil {
		apperrors.WriteError(w, apperrors.ErrServiceUnavailable.WithMessage("Rate limiter unavailable"))
		return false
	}
	middleware.WriteRateLimitHeaders(w, namespace, policy, decision)
	if decision.Allowed {
		return true
	}
	retry := int((decision.RetryAfter + time.Second - 1) / time.Second)
	if retry < 1 {
		retry = 1
	}
	w.Header().Set("Retry-After", strconv.Itoa(retry))
	httpx.WriteMappedError(w, ErrRateLimited)
	return false
}
func devicePollKey(code string) string {
	sum := sha256.Sum256([]byte(strings.TrimSpace(code)))
	return hex.EncodeToString(sum[:])
}
