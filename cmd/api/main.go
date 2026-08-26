package main

import (
	"fmt"
	"os"

	"github.com/box1o/woki/internal/application"
	"github.com/box1o/woki/pkg/config"
	"github.com/box1o/woki/pkg/log"
	"github.com/box1o/woki/pkg/shutdown"
)

func main() {
	exitCode := 0
	if err := run(); err != nil {
		log.Error("%v", err)
		exitCode = 1
	}
	if err := log.Cleanup(); err != nil {
		fmt.Fprintf(os.Stderr, "failed to close logger: %v\n", err)
		exitCode = 1
	}
	if exitCode != 0 {
		os.Exit(exitCode)
	}
}

func run() error {
	cfg, err := config.Load()
	if err != nil {
		return err
	}
	if err := log.Configure(log.Config{
		Output:        cfg.Log.Output,
		Level:         cfg.Log.Level,
		FilePath:      cfg.Log.FilePath,
		DisableColors: cfg.Log.DisableColors,
	}); err != nil {
		return config.ErrLogConfig.WithErr(err)
	}

	app, err := application.New(cfg)
	if err != nil {
		return err
	}
	serverErr := make(chan error, 1)
	go func() { serverErr <- app.Run() }()

	shutdownErr := make(chan error, 1)
	go func() { shutdownErr <- shutdown.Wait(app, cfg.Shutdown.Timeout) }()

	select {
	case err := <-serverErr:
		return err
	case err := <-shutdownErr:
		return err
	}
}
