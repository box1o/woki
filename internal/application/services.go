package application

import (
	"context"

	domainmail "github.com/box1o/woki/internal/domain/mail"
	"github.com/box1o/woki/internal/domain/user"
	inframail "github.com/box1o/woki/internal/infrastructure/mail"
	"github.com/box1o/woki/internal/infrastructure/session"
	authsvc "github.com/box1o/woki/internal/services/auth"
	"github.com/box1o/woki/internal/services/deviceauth"
	mailsvc "github.com/box1o/woki/internal/services/mail"
	workspacesvc "github.com/box1o/woki/internal/services/workspace"
)

func (a *Application) setupServices() error {
	a.workspaceSvc = workspacesvc.NewWithEvents(a.workspaceRepo, a.userRepo, a.eventBus)
	var sessions authsvc.SessionManager
	if a.redis != nil {
		sessions = session.NewRedis(a.redis, a.cfg.Session.TTL, a.cfg.Session.RedisPrefix)
	} else {
		sessions = session.New(a.cfg.Session.TTL)
	}
	a.authSvc = authsvc.NewWithEvents(a.userRepo, sessions, func(ctx context.Context, usr *user.User) error {
		_, err := a.workspaceSvc.EnsurePersonal(ctx, usr)
		return err
	}, a.eventBus)
	if a.redis != nil {
		a.deviceSvc = deviceauth.NewRedisService(a.cfg.Frontend.URL, a.cfg.DeviceAuth.CodeTTL, a.cfg.DeviceAuth.CredentialTTL, a.cfg.DeviceAuth.RedisPrefix, a.redis, a.credentialRepo, a.userRepo)
	} else {
		a.deviceSvc = deviceauth.New(a.cfg.Frontend.URL, a.cfg.DeviceAuth.CodeTTL, a.cfg.DeviceAuth.CredentialTTL, a.credentialRepo, a.userRepo)
	}
	var sender domainmail.Sender
	if a.cfg.Mail.Enabled {
		var err error
		sender, err = inframail.NewSMTP(a.cfg.Mail)
		if err != nil {
			return err
		}
	}
	a.mailSvc = mailsvc.New(a.cfg.Mail, a.cfg.Frontend.URL, sender, a.eventBus)
	return a.mailSvc.Setup()
}
