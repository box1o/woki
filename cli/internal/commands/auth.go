package commands

import (
	"context"
	"fmt"

	"github.com/box1o/woki/cli/internal/auth"
	"github.com/box1o/woki/cli/internal/ui"
	"github.com/box1o/woki/pkg/api"
)

func (a *App) runAuth(ctx context.Context, p *ui.Presenter, s *auth.Service, args []string) int {
	if len(args) == 0 {
		return a.fail(usage("auth command is required"))
	}
	switch args[0] {
	case "login":
		credential, err := s.Login(ctx, "Woki CLI", func(code api.DeviceCodeResponse) error {
			return p.LoginInstructions(code.VerificationURI, code.UserCode)
		})
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Authentication successful", [2]string{"User", credential.Owner.Email}, [2]string{"Expires", credential.ExpiresAt.Format("2006-01-02 15:04 MST")}); err != nil {
			return a.fail(err)
		}
		return 0
	case "status":
		credential, status, err := s.Status(ctx)
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Authenticated", [2]string{"User", credential.Owner.Email}, [2]string{"Client", fmt.Sprint(status["client_name"])}, [2]string{"Expires", credential.ExpiresAt.Format("2006-01-02 15:04 MST")}); err != nil {
			return a.fail(err)
		}
		return 0
	case "logout":
		if err := s.Logout(ctx); err != nil {
			return a.fail(err)
		}
		if err := p.Success("Logged out"); err != nil {
			return a.fail(err)
		}
		return 0
	default:
		return a.fail(usage("unknown auth command %q", args[0]))
	}
}
