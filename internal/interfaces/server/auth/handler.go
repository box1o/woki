package auth

import (
	"context"
	"crypto/rand"
	"crypto/subtle"
	"encoding/base64"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/infrastructure/provider"
	"github.com/box1o/woki/internal/interfaces/server/middleware"
	authsvc "github.com/box1o/woki/internal/services/auth"
	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/config"
	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/httpx"
	"github.com/box1o/woki/pkg/log"
	contract "github.com/box1o/woki/pkg/ratelimit"
)

type OAuthProvider interface {
	Configured() bool
	AuthURL(string) string
	Exchange(context.Context, string) (provider.Profile, error)
}

type Service interface {
	Login(context.Context, authsvc.Profile) (*user.User, string, error)
	Logout(context.Context, string) error
}

type Handler struct {
	service     Service
	google      OAuthProvider
	auth        *middleware.Authenticator
	frontendURL string
	devEnabled  bool
	cookie      config.CookieConfig
	sessionTTL  time.Duration
	limiter     contract.Limiter
	rate        config.RateLimitConfig
}

func New(service Service, google OAuthProvider, auth *middleware.Authenticator, frontendURL string, devEnabled bool, cookie config.CookieConfig, sessionTTL time.Duration) *Handler {
	return NewWithLimiter(service, google, auth, frontendURL, devEnabled, cookie, sessionTTL, nil, config.RateLimitConfig{})
}

func NewWithLimiter(service Service, google OAuthProvider, auth *middleware.Authenticator, frontendURL string, devEnabled bool, cookie config.CookieConfig, sessionTTL time.Duration, limiter contract.Limiter, rate config.RateLimitConfig) *Handler {
	return &Handler{service: service, google: google, auth: auth, frontendURL: strings.TrimRight(frontendURL, "/"), devEnabled: devEnabled, cookie: cookie, sessionTTL: sessionTTL, limiter: limiter, rate: rate}
}

func (h *Handler) Register(mux *http.ServeMux) {
	limit := func(name string, handler http.Handler) http.Handler {
		if !h.rate.Enabled {
			return handler
		}
		return middleware.RateLimit(h.limiter, h.rate.Prefix+":auth:"+name, contract.Policy{Rate: h.rate.AuthLimit, Burst: h.rate.AuthBurst, Period: h.rate.AuthWindow}, true)(handler)
	}
	mux.Handle("GET /auth/google", limit("google-start", http.HandlerFunc(h.startGoogle)))
	mux.Handle("GET /auth/google/callback", limit("google-callback", http.HandlerFunc(h.finishGoogle)))
	mux.Handle("POST /auth/dev", limit("dev", http.HandlerFunc(h.devLogin)))
	mux.Handle("GET /auth/status", h.auth.RequireWeb(http.HandlerFunc(h.status)))
	mux.Handle("POST /auth/logout", h.auth.RequireWeb(http.HandlerFunc(h.logout)))
}

func (h *Handler) startGoogle(w http.ResponseWriter, r *http.Request) {
	if h.google == nil || !h.google.Configured() {
		apperrors.WriteError(w, ErrOAuthNotConfigured.WithMessage("Google authentication is not configured"))
		return
	}
	state, err := randomState()
	if err != nil {
		log.Error("generate Google OAuth state: %v", err)
		apperrors.WriteError(w, apperrors.ErrInternalServer)
		return
	}
	returnTo := safeReturnPath(r.URL.Query().Get("return_to"))
	expires := time.Now().Add(10 * time.Minute)
	setOAuthCookie(w, oauthStateCookie(), state, "/auth/google/callback", h.cookie.Secure, expires)
	setOAuthCookie(w, oauthReturnCookie(), returnTo, "/auth/google/callback", h.cookie.Secure, expires)
	http.Redirect(w, r, h.google.AuthURL(state), http.StatusFound)
}

func (h *Handler) finishGoogle(w http.ResponseWriter, r *http.Request) {
	if h.google == nil || !h.google.Configured() {
		apperrors.WriteError(w, ErrOAuthNotConfigured)
		return
	}
	stateCookie, err := r.Cookie(oauthStateCookie())
	queryState := r.URL.Query().Get("state")
	if err != nil || !equalState(stateCookie.Value, queryState) {
		h.clearOAuthCookies(w)
		h.redirectOAuthError(w, r, "/", "state")
		return
	}
	returnTo := "/"
	if returnCookie, cookieErr := r.Cookie(oauthReturnCookie()); cookieErr == nil {
		returnTo = safeReturnPath(returnCookie.Value)
	}
	h.clearOAuthCookies(w)
	if denied := strings.TrimSpace(r.URL.Query().Get("error")); denied != "" {
		log.Warn("Google OAuth denied: %s", denied)
		h.redirectOAuthError(w, r, returnTo, "denied")
		return
	}
	code := strings.TrimSpace(r.URL.Query().Get("code"))
	if code == "" {
		h.redirectOAuthError(w, r, returnTo, "invalid_response")
		return
	}
	profile, err := h.google.Exchange(r.Context(), code)
	if err != nil {
		log.Warn("Google OAuth exchange failed: %v", err)
		h.redirectOAuthError(w, r, returnTo, "provider")
		return
	}
	usr, token, err := h.service.Login(r.Context(), authsvc.Profile{Email: profile.Email, Name: profile.Name, AvatarURL: profile.AvatarURL, Provider: profile.Provider, ProviderID: profile.ProviderID})
	if err != nil {
		log.Error("complete Google login: %v", err)
		if apperrors.IsCode(err, authsvc.ErrIdentityConflict.Code) {
			h.redirectOAuthError(w, r, returnTo, "identity_conflict")
		} else {
			h.redirectOAuthError(w, r, returnTo, "unavailable")
		}
		return
	}
	if usr == nil || token == "" {
		log.Error("complete Google login returned an empty identity or session")
		h.redirectOAuthError(w, r, returnTo, "unavailable")
		return
	}
	h.setSessionCookie(w, token)
	http.Redirect(w, r, h.frontendURL+returnTo, http.StatusFound)
}

func (h *Handler) redirectOAuthError(w http.ResponseWriter, r *http.Request, returnTo, code string) {
	values := url.Values{"oauth_error": {strings.TrimSpace(code)}}
	if safe := safeReturnPath(returnTo); safe != "/" {
		values.Set("return_to", safe)
	}
	http.Redirect(w, r, h.frontendURL+"/auth?"+values.Encode(), http.StatusSeeOther)
}

func (h *Handler) devLogin(w http.ResponseWriter, r *http.Request) {
	if !h.devEnabled {
		apperrors.WriteError(w, ErrDevAuthDisabled)
		return
	}
	var req struct {
		Email string `json:"email"`
		Name  string `json:"name"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		apperrors.Write(w, err)
		return
	}
	usr, token, err := h.service.Login(r.Context(), authsvc.Profile{Email: req.Email, Name: req.Name, Provider: user.ProviderDev, ProviderID: strings.ToLower(strings.TrimSpace(req.Email))})
	if err != nil {
		apperrors.Write(w, err)
		return
	}
	h.setSessionCookie(w, token)
	httpx.WriteJSON(w, http.StatusOK, api.AuthStatus{Authenticated: true, User: toAPIUser(usr)})
}
func (h *Handler) status(w http.ResponseWriter, r *http.Request) {
	principal, ok := middleware.PrincipalFrom(r.Context())
	if !ok || principal.User == nil {
		apperrors.WriteError(w, apperrors.ErrUnauthorized)
		return
	}
	httpx.WriteJSON(w, http.StatusOK, api.AuthStatus{Authenticated: true, User: toAPIUser(principal.User)})
}
func (h *Handler) logout(w http.ResponseWriter, r *http.Request) {
	principal, ok := middleware.PrincipalFrom(r.Context())
	if ok && principal.WebSession != "" {
		if err := h.service.Logout(r.Context(), principal.WebSession); err != nil {
			log.Warn("delete browser session during logout: %v", err)
		}
	}
	clearCookie(w, h.cookie.Name, "/", h.cookie.Secure, h.sessionSameSite())
	w.WriteHeader(http.StatusNoContent)
}
func (h *Handler) setSessionCookie(w http.ResponseWriter, token string) {
	http.SetCookie(w, &http.Cookie{Name: h.cookie.Name, Value: token, Path: "/", HttpOnly: true, Secure: h.cookie.Secure, SameSite: h.sessionSameSite(), MaxAge: int(h.sessionTTL.Seconds()), Expires: time.Now().Add(h.sessionTTL)})
}
func (h *Handler) sessionSameSite() http.SameSite {
	switch strings.ToLower(h.cookie.SameSite) {
	case "lax":
		return http.SameSiteLaxMode
	case "none":
		return http.SameSiteNoneMode
	default:
		return http.SameSiteStrictMode
	}
}
func setOAuthCookie(w http.ResponseWriter, name, value, path string, secure bool, expires time.Time) {
	http.SetCookie(w, &http.Cookie{Name: name, Value: value, Path: path, HttpOnly: true, Secure: secure, SameSite: http.SameSiteLaxMode, MaxAge: 600, Expires: expires})
}
func clearCookie(w http.ResponseWriter, name, path string, secure bool, sameSite http.SameSite) {
	http.SetCookie(w, &http.Cookie{Name: name, Value: "", Path: path, HttpOnly: true, Secure: secure, SameSite: sameSite, MaxAge: -1, Expires: time.Unix(1, 0)})
}
func (h *Handler) clearOAuthCookies(w http.ResponseWriter) {
	clearCookie(w, oauthStateCookie(), "/auth/google/callback", h.cookie.Secure, http.SameSiteLaxMode)
	clearCookie(w, oauthReturnCookie(), "/auth/google/callback", h.cookie.Secure, http.SameSiteLaxMode)
}
func oauthStateCookie() string  { return "woki_oauth_state_google" }
func oauthReturnCookie() string { return "woki_oauth_return_to_google" }
func equalState(cookieState, queryState string) bool {
	if cookieState == "" || len(cookieState) != len(queryState) {
		return false
	}
	return subtle.ConstantTimeCompare([]byte(cookieState), []byte(queryState)) == 1
}
func safeReturnPath(value string) string {
	value = strings.TrimSpace(value)
	if value == "" || !strings.HasPrefix(value, "/") || strings.HasPrefix(value, "//") {
		return "/"
	}
	u, err := url.Parse(value)
	if err != nil || u.IsAbs() || u.Host != "" || u.Fragment != "" {
		return "/"
	}
	decodedPath, err := url.PathUnescape(u.EscapedPath())
	if err != nil || strings.Contains(decodedPath, "\\") {
		return "/"
	}
	return u.RequestURI()
}
func randomState() (string, error) {
	buf := make([]byte, 32)
	if _, err := rand.Read(buf); err != nil {
		return "", err
	}
	return base64.RawURLEncoding.EncodeToString(buf), nil
}
func toAPIUser(u *user.User) *api.User {
	if u == nil {
		return nil
	}
	return &api.User{ID: u.ID.String(), Email: u.Email, Name: u.Name, AvatarURL: u.AvatarURL}
}
