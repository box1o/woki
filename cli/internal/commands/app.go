// Package commands owns CLI parsing and command composition.
package commands

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"os/signal"
	"strings"
	"syscall"

	client "github.com/box1o/woki/cli/internal/api"
	authsvc "github.com/box1o/woki/cli/internal/auth"
	cliconfig "github.com/box1o/woki/cli/internal/config"
	"github.com/box1o/woki/cli/internal/credentials"
	"github.com/box1o/woki/cli/internal/platform"
	"github.com/box1o/woki/cli/internal/ui"
	workspacesvc "github.com/box1o/woki/cli/internal/workspace"
)

type App struct {
	in          io.Reader
	out, errOut io.Writer
}

func New(out, errOut io.Writer) *App { return &App{in: os.Stdin, out: out, errOut: errOut} }

func newWithInput(in io.Reader, out, errOut io.Writer) *App {
	return &App{in: in, out: out, errOut: errOut}
}
func (a *App) Run(ctx context.Context, args []string) int {
	global := flag.NewFlagSet("woki", flag.ContinueOnError)
	global.SetOutput(a.errOut)
	apiURL := global.String("api-url", "", "API base URL")
	jsonOut := global.Bool("json", false, "emit JSON")
	quiet := global.Bool("quiet", false, "suppress output")
	global.BoolVar(quiet, "q", false, "suppress output")
	noColor := global.Bool("no-color", false, "disable ANSI color")
	noInteractive := global.Bool("no-interactive", false, "do not open a browser")
	if err := global.Parse(args); err != nil {
		return 2
	}
	rest := global.Args()
	if len(rest) == 0 || rest[0] == "help" || rest[0] == "--help" {
		a.help()
		return 0
	}
	if rest[0] == "version" {
		fmt.Fprintln(a.out, "woki dev")
		return 0
	}
	cfg, err := cliconfig.Load(*apiURL)
	if err != nil {
		return a.fail(err)
	}
	apiClient, err := client.New(cfg.APIURL)
	if err != nil {
		return a.fail(err)
	}
	store, err := credentials.New()
	if err != nil {
		return a.fail(err)
	}
	presenter := ui.New(a.out, ui.Mode{JSON: *jsonOut, Quiet: *quiet, NoColor: *noColor})
	var browser authsvc.Browser = platform.Browser{}
	if *noInteractive {
		browser = nil
	}
	authService := authsvc.New(apiClient, store, browser)
	currentStore, err := workspacesvc.NewCurrentStore()
	if err != nil {
		return a.fail(err)
	}
	workspaceService := workspacesvc.New(apiClient, store, currentStore)
	switch rest[0] {
	case "auth":
		return a.runAuth(ctx, presenter, authService, rest[1:])
	case "workspace":
		return a.runWorkspace(ctx, presenter, workspaceService, rest[1:], !*noInteractive && presenter.AllowsInteractive() && ui.CanPrompt(a.in))
	default:
		return a.fail(usage("unknown command %q", rest[0]))
	}
}
func (a *App) fail(err error) int {
	if err == nil {
		return 0
	}
	fmt.Fprintln(a.errOut, "error:", err)
	var apiErr *client.Error
	if errors.As(err, &apiErr) {
		if apiErr.Status == 401 || apiErr.Status == 403 {
			return 4
		}
	}
	if errors.Is(err, context.Canceled) {
		return 130
	}
	if errors.Is(err, credentials.ErrNotFound) {
		return 4
	}
	if errors.Is(err, ErrUsage) {
		return 2
	}
	return 1
}
func (a *App) help() {
	fmt.Fprintln(a.out, strings.TrimSpace(`Woki CLI

Usage:
  woki [global options] auth <login|status|logout>
  woki [global options] workspace <list|use|current|create|delete|members|users|add|remove|role>

Workspace workflow:
  woki workspace use              Select the current workspace from a menu
  woki workspace use <name>       Select it by name
  woki workspace current          Show the current workspace
  woki workspace members          Use the current workspace implicitly
  woki workspace add              Search and select an existing user

Global options:
  --api-url URL
  --json
  --quiet, -q
  --no-color
  --no-interactive`))
}
func Main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	os.Exit(New(os.Stdout, os.Stderr).Run(ctx, os.Args[1:]))
}
