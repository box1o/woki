package middleware

import (
	"context"
	"net/http"
	"strings"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	apperrors "github.com/box1o/woki/pkg/errors"
)

type BrowserAuth interface {
	UserFromSession(context.Context, string) (*user.User, error)
}

type CLIAuth interface {
	Authenticate(context.Context, string) (*user.User, *domaincli.Credential, error)
}

type Principal struct {
	User       *user.User
	Credential *domaincli.Credential
	Token      string
	WebSession string
}

type principalKey struct{}

type Authenticator struct {
	browser       BrowserAuth
	cli           CLIAuth
	cookieName    string
	allowedOrigin string
}

func NewAuthenticator(
	browser BrowserAuth,
	cli CLIAuth,
	cookieName, allowedOrigin string,
) *Authenticator {
	return &Authenticator{
		browser:       browser,
		cli:           cli,
		cookieName:    cookieName,
		allowedOrigin: strings.TrimRight(strings.TrimSpace(allowedOrigin), "/"),
	}
}

func (a *Authenticator) RequireWeb(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if !a.webOriginAllowed(r) {
			apperrors.WriteError(w, apperrors.ErrForbidden.WithMessage("Request origin is not allowed"))
			return
		}
		principal, err := a.webPrincipal(r)
		if err != nil {
			apperrors.Write(w, err)
			return
		}
		next.ServeHTTP(w, r.WithContext(context.WithValue(r.Context(), principalKey{}, principal)))
	})
}

func (a *Authenticator) RequireCLI(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		principal, err := a.cliPrincipal(r)
		if err != nil {
			apperrors.Write(w, err)
			return
		}
		next.ServeHTTP(w, r.WithContext(context.WithValue(r.Context(), principalKey{}, principal)))
	})
}

func (a *Authenticator) RequireAny(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if strings.TrimSpace(r.Header.Get("Authorization")) != "" {
			principal, err := a.cliPrincipal(r)
			if err != nil {
				apperrors.Write(w, err)
				return
			}
			next.ServeHTTP(w, r.WithContext(context.WithValue(r.Context(), principalKey{}, principal)))
			return
		}

		if !a.webOriginAllowed(r) {
			apperrors.WriteError(w, apperrors.ErrForbidden.WithMessage("Request origin is not allowed"))
			return
		}
		principal, err := a.webPrincipal(r)
		if err != nil {
			apperrors.Write(w, err)
			return
		}
		next.ServeHTTP(w, r.WithContext(context.WithValue(r.Context(), principalKey{}, principal)))
	})
}

func (a *Authenticator) webPrincipal(r *http.Request) (Principal, error) {
	cookie, err := r.Cookie(a.cookieName)
	if err != nil || strings.TrimSpace(cookie.Value) == "" {
		return Principal{}, ErrSessionMissing
	}
	usr, err := a.browser.UserFromSession(r.Context(), cookie.Value)
	if err != nil {
		return Principal{}, err
	}
	return Principal{User: usr, WebSession: cookie.Value}, nil
}

func (a *Authenticator) cliPrincipal(r *http.Request) (Principal, error) {
	token, ok := bearerToken(r.Header.Get("Authorization"))
	if !ok {
		return Principal{}, ErrBearerMissing
	}
	usr, credential, err := a.cli.Authenticate(r.Context(), token)
	if err != nil {
		return Principal{}, err
	}
	return Principal{User: usr, Credential: credential, Token: token}, nil
}

func (a *Authenticator) webOriginAllowed(r *http.Request) bool {
	switch r.Method {
	case http.MethodGet, http.MethodHead, http.MethodOptions:
		return true
	}
	origin := strings.TrimRight(strings.TrimSpace(r.Header.Get("Origin")), "/")
	return origin != "" && origin == a.allowedOrigin
}

func PrincipalFrom(ctx context.Context) (Principal, bool) {
	p, ok := ctx.Value(principalKey{}).(Principal)
	return p, ok
}

func bearerToken(value string) (string, bool) {
	parts := strings.Fields(value)
	if len(parts) != 2 || !strings.EqualFold(parts[0], "Bearer") || parts[1] == "" {
		return "", false
	}
	return parts[1], true
}
