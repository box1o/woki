package application

import (
	"github.com/box1o/woki/internal/infrastructure/provider"
	server "github.com/box1o/woki/internal/interfaces/server"
	authhttp "github.com/box1o/woki/internal/interfaces/server/auth"
	devicehttp "github.com/box1o/woki/internal/interfaces/server/deviceauth"
	"github.com/box1o/woki/internal/interfaces/server/health"
	mailhttp "github.com/box1o/woki/internal/interfaces/server/mail"
	"github.com/box1o/woki/internal/interfaces/server/middleware"
	workspacehttp "github.com/box1o/woki/internal/interfaces/server/workspace"
)

func (a *Application) setupServer() error {
	checks := map[string]health.Checker{}
	if a.db != nil {
		checks["postgres"] = a.db
	}
	if a.redis != nil {
		checks["redis"] = a.redis
	}
	authenticator := middleware.NewAuthenticator(a.authSvc, a.deviceSvc, a.cfg.Auth.Cookie.Name, a.cfg.CORS.AllowedOrigin)
	google := provider.NewGoogle(a.cfg.Auth.Google)
	github := provider.NewGitHub(a.cfg.Auth.GitHub)
	authHandler := authhttp.NewWithProvidersAndLimiter(a.authSvc, google, github, authenticator, a.cfg.Frontend.URL, a.cfg.Auth.Dev, a.cfg.Auth.Cookie, a.cfg.Session.TTL, a.limiter, a.cfg.RateLimit)
	deviceHandler := devicehttp.NewWithLimiter(a.deviceSvc, authenticator, a.limiter, a.cfg.RateLimit)
	workspaceHandler := workspacehttp.New(a.workspaceSvc, authenticator)
	mailHandler := mailhttp.NewWithLimiter(a.mailSvc, authenticator, a.limiter, a.cfg.RateLimit)
	a.server = server.NewWithRateLimiter(a.cfg.Server, a.cfg.CORS.AllowedOrigin, a.limiter, a.cfg.RateLimit, health.New(checks), authHandler, deviceHandler, workspaceHandler, mailHandler)
	return nil
}
