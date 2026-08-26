// Package application is the explicit composition root for Woki.
package application

import (
	"context"
	stderrors "errors"
	"net/http"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/internal/infrastructure/db/postgres"
	eventinfra "github.com/box1o/woki/internal/infrastructure/events"
	"github.com/box1o/woki/internal/infrastructure/file"
	infraredis "github.com/box1o/woki/internal/infrastructure/redis"
	server "github.com/box1o/woki/internal/interfaces/server"
	devicehttp "github.com/box1o/woki/internal/interfaces/server/deviceauth"
	"github.com/box1o/woki/internal/interfaces/server/middleware"
	authsvc "github.com/box1o/woki/internal/services/auth"
	mailsvc "github.com/box1o/woki/internal/services/mail"
	workspacesvc "github.com/box1o/woki/internal/services/workspace"
	"github.com/box1o/woki/pkg/config"
	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/log"
	contract "github.com/box1o/woki/pkg/ratelimit"
)

type deviceRuntime interface {
	devicehttp.Service
	middleware.CLIAuth
}

type Application struct {
	cfg    config.Config
	server *server.Server

	db        *postgres.Database
	redis     *infraredis.Client
	fileStore *file.Store
	eventBus  *eventinfra.Bus
	limiter   contract.Limiter

	userRepo       user.Repository
	workspaceRepo  workspace.Repository
	credentialRepo domaincli.Repository

	workspaceSvc *workspacesvc.Service
	authSvc      *authsvc.Service
	deviceSvc    deviceRuntime
	mailSvc      *mailsvc.Service
}

func New(cfg config.Config) (*Application, error) {
	if err := cfg.Validate(); err != nil {
		return nil, apperrors.Wrap(err, "APPLICATION_CONFIG_FAILED", "failed to validate application configuration")
	}
	a := &Application{cfg: cfg}
	if err := a.initialize(); err != nil {
		_ = a.closeInfrastructure(context.Background())
		return nil, err
	}
	return a, nil
}

func (a *Application) initialize() error {
	steps := []struct {
		name string
		fn   func() error
	}{
		{"infrastructure", a.setupInfrastructure},
		{"repositories", a.setupRepositories},
		{"services", a.setupServices},
		{"server", a.setupServer},
	}
	for _, step := range steps {
		if err := step.fn(); err != nil {
			return apperrors.Wrap(err, "APPLICATION_INITIALIZATION_FAILED", step.name+" initialization failed")
		}
	}
	return nil
}

func (a *Application) Run() error {
	log.Info("Woki API listening on %s", a.cfg.Server.Addr)
	if err := a.server.Start(); err != nil {
		return apperrors.Wrap(err, "APPLICATION_SERVER_FAILED", "Woki API server stopped unexpectedly")
	}
	return nil
}
func (a *Application) Shutdown(ctx context.Context) error {
	var errs []error
	if a.server != nil {
		if err := a.server.Shutdown(ctx); err != nil {
			errs = append(errs, err)
		}
	}
	if err := a.closeInfrastructure(ctx); err != nil {
		errs = append(errs, err)
	}
	if len(errs) > 0 {
		return apperrors.Wrap(stderrors.Join(errs...), "APPLICATION_SHUTDOWN_FAILED", "failed to shut down Woki")
	}
	return nil
}
func (a *Application) closeInfrastructure(ctx context.Context) error {
	var errs []error
	if a.mailSvc != nil {
		if err := a.mailSvc.Shutdown(ctx); err != nil {
			errs = append(errs, err)
		}
		a.mailSvc = nil
	}
	if a.redis != nil {
		if err := a.redis.Shutdown(ctx); err != nil {
			errs = append(errs, err)
		}
		a.redis = nil
	}
	if a.db != nil {
		if err := a.db.Shutdown(ctx); err != nil {
			errs = append(errs, err)
		}
		a.db = nil
	}
	return stderrors.Join(errs...)
}
func (a *Application) Handler() http.Handler {
	if a.server == nil {
		return http.NotFoundHandler()
	}
	return a.server.Handler()
}
