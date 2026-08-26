package commands

import (
	"context"
	"flag"
	"strings"

	"github.com/box1o/woki/cli/internal/ui"
	service "github.com/box1o/woki/cli/internal/workspace"
	"github.com/box1o/woki/pkg/api"
)

func (a *App) runWorkspace(ctx context.Context, p *ui.Presenter, s *service.Service, args []string, interactive bool) int {
	if len(args) == 0 {
		return a.fail(usage("workspace command is required"))
	}
	switch args[0] {
	case "list":
		if len(args) != 1 {
			return a.fail(usage("woki workspace list"))
		}
		values, err := s.List(ctx)
		if err != nil {
			return a.fail(err)
		}
		currentID := ""
		if current, err := s.Selection(ctx); err == nil {
			currentID = current.ID
		}
		rows := make([][]string, 0, len(values))
		for _, v := range values {
			marker := ""
			if v.ID == currentID {
				marker = "yes"
			}
			rows = append(rows, []string{v.Name, marker, v.ID, v.OwnerID})
		}
		if pErr := p.Table([]string{"NAME", "CURRENT", "ID", "OWNER"}, rows); pErr != nil {
			return a.fail(pErr)
		}
		return 0

	case "use":
		if len(args) > 2 {
			return a.fail(usage("woki workspace use [name]"))
		}
		var selected api.Workspace
		var err error
		if len(args) == 2 {
			selected, err = s.SetCurrent(ctx, args[1])
		} else {
			if !interactive {
				return a.fail(usage("woki workspace use <name> (interactive terminal required when name is omitted)"))
			}
			selected, err = a.chooseWorkspace(ctx, s)
			if err == nil {
				selected, err = s.SetCurrent(ctx, selected.ID)
			}
		}
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Current workspace updated", [2]string{"Name", selected.Name}, [2]string{"ID", selected.ID}); err != nil {
			return a.fail(err)
		}
		return 0

	case "current":
		if len(args) != 1 {
			return a.fail(usage("woki workspace current"))
		}
		workspace, err := s.Current(ctx)
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Current workspace", [2]string{"Name", workspace.Name}, [2]string{"ID", workspace.ID}); err != nil {
			return a.fail(err)
		}
		return 0

	case "create":
		fs := flag.NewFlagSet("workspace create", flag.ContinueOnError)
		fs.SetOutput(a.errOut)
		use := fs.Bool("use", false, "set the new workspace as current")
		if err := fs.Parse(args[1:]); err != nil {
			return 2
		}
		if len(fs.Args()) != 1 {
			return a.fail(usage("woki workspace create [--use] <name>"))
		}
		v, err := s.Create(ctx, fs.Args()[0])
		if err != nil {
			return a.fail(err)
		}
		if *use {
			if _, err := s.SetCurrent(ctx, v.ID); err != nil {
				return a.fail(err)
			}
		}
		fields := [][2]string{{"ID", v.ID}, {"Name", v.Name}}
		if *use {
			fields = append(fields, [2]string{"Current", "yes"})
		}
		if err := p.Success("Workspace created", fields...); err != nil {
			return a.fail(err)
		}
		return 0

	case "delete":
		if len(args) > 2 {
			return a.fail(usage("woki workspace delete [workspace]"))
		}
		ref := optionalArg(args[1:])
		workspace, err := s.Delete(ctx, ref)
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Workspace deleted", [2]string{"Name", workspace.Name}, [2]string{"ID", workspace.ID}); err != nil {
			return a.fail(err)
		}
		return 0

	case "members":
		if len(args) > 2 {
			return a.fail(usage("woki workspace members [workspace]"))
		}
		values, err := s.Members(ctx, optionalArg(args[1:]))
		if err != nil {
			return a.fail(err)
		}
		rows := make([][]string, 0, len(values))
		for _, m := range values {
			rows = append(rows, []string{m.Name, m.Email, m.Role, m.ID})
		}
		if err := p.Table([]string{"NAME", "EMAIL", "ROLE", "ID"}, rows); err != nil {
			return a.fail(err)
		}
		return 0

	case "users":
		fs := flag.NewFlagSet("workspace users", flag.ContinueOnError)
		fs.SetOutput(a.errOut)
		workspace := fs.String("workspace", "", "workspace name or ID; defaults to current")
		limit := fs.Int("limit", 8, "maximum results")
		if err := fs.Parse(args[1:]); err != nil {
			return 2
		}
		if len(fs.Args()) != 1 {
			return a.fail(usage("woki workspace users [--workspace name] <query>"))
		}
		values, err := s.Candidates(ctx, *workspace, fs.Args()[0], *limit)
		if err != nil {
			return a.fail(err)
		}
		rows := make([][]string, 0, len(values))
		for _, user := range values {
			rows = append(rows, []string{user.Name, user.Email, user.ID})
		}
		if err := p.Table([]string{"NAME", "EMAIL", "ID"}, rows); err != nil {
			return a.fail(err)
		}
		return 0

	case "add":
		fs := flag.NewFlagSet("workspace add", flag.ContinueOnError)
		fs.SetOutput(a.errOut)
		role := fs.String("role", "member", "member role")
		workspace := fs.String("workspace", "", "workspace name or ID; defaults to current")
		if err := fs.Parse(args[1:]); err != nil {
			return 2
		}
		rest := fs.Args()
		if len(rest) > 1 {
			return a.fail(usage("woki workspace add [--workspace name] [--role member] [email]"))
		}
		email := optionalArg(rest)
		if email == "" {
			if !interactive {
				return a.fail(usage("woki workspace add [--workspace name] <email>"))
			}
			selected, err := a.chooseUser(ctx, s, *workspace)
			if err != nil {
				return a.fail(err)
			}
			email = selected.Email
		}
		m, err := s.Add(ctx, *workspace, email, *role)
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Member added", [2]string{"Name", m.Name}, [2]string{"Email", m.Email}, [2]string{"Role", m.Role}); err != nil {
			return a.fail(err)
		}
		return 0

	case "remove":
		fs := flag.NewFlagSet("workspace remove", flag.ContinueOnError)
		fs.SetOutput(a.errOut)
		workspace := fs.String("workspace", "", "workspace name or ID; defaults to current")
		if err := fs.Parse(args[1:]); err != nil {
			return 2
		}
		if len(fs.Args()) != 1 {
			return a.fail(usage("woki workspace remove [--workspace name] <email-or-member-id>"))
		}
		member, err := s.Remove(ctx, *workspace, fs.Args()[0])
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Member removed", [2]string{"Email", member.Email}); err != nil {
			return a.fail(err)
		}
		return 0

	case "role":
		fs := flag.NewFlagSet("workspace role", flag.ContinueOnError)
		fs.SetOutput(a.errOut)
		role := fs.String("role", "", "new role")
		workspace := fs.String("workspace", "", "workspace name or ID; defaults to current")
		if err := fs.Parse(args[1:]); err != nil {
			return 2
		}
		if len(fs.Args()) != 1 || strings.TrimSpace(*role) == "" {
			return a.fail(usage("woki workspace role --role admin [--workspace name] <email-or-member-id>"))
		}
		m, err := s.Role(ctx, *workspace, fs.Args()[0], *role)
		if err != nil {
			return a.fail(err)
		}
		if err := p.Success("Member role updated", [2]string{"Email", m.Email}, [2]string{"Role", m.Role}); err != nil {
			return a.fail(err)
		}
		return 0

	default:
		return a.fail(usage("unknown workspace command %q", args[0]))
	}
}

func (a *App) chooseWorkspace(ctx context.Context, s *service.Service) (api.Workspace, error) {
	values, err := s.List(ctx)
	if err != nil {
		return api.Workspace{}, err
	}
	if len(values) == 0 {
		return api.Workspace{}, service.ErrWorkspaceNotFound.WithDetail("no workspaces are available")
	}
	currentID := ""
	if current, err := s.Selection(ctx); err == nil {
		currentID = current.ID
	}
	options := make([]ui.Option, 0, len(values))
	currentIndex := -1
	for i, workspace := range values {
		options = append(options, ui.Option{Label: workspace.Name, Description: workspace.ID})
		if workspace.ID == currentID {
			currentIndex = i
		}
	}
	index, err := ui.Select(a.in, a.out, "Select current workspace", options, currentIndex)
	if err != nil {
		return api.Workspace{}, err
	}
	return values[index], nil
}

func (a *App) chooseUser(ctx context.Context, s *service.Service, workspace string) (api.User, error) {
	query, err := ui.PromptLine(a.in, a.out, "Search user by name or email")
	if err != nil {
		return api.User{}, err
	}
	if strings.TrimSpace(query) == "" {
		return api.User{}, usage("user search cannot be empty")
	}
	values, err := s.Candidates(ctx, workspace, query, 10)
	if err != nil {
		return api.User{}, err
	}
	if len(values) == 0 {
		return api.User{}, service.ErrCandidateNotFound.WithDetail(query)
	}
	options := make([]ui.Option, 0, len(values))
	for _, user := range values {
		options = append(options, ui.Option{Label: user.Name, Description: user.Email})
	}
	index, err := ui.Select(a.in, a.out, "Select user", options, -1)
	if err != nil {
		return api.User{}, err
	}
	return values[index], nil
}

func optionalArg(args []string) string {
	if len(args) == 0 {
		return ""
	}
	return strings.TrimSpace(args[0])
}
