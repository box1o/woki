package application

import (
	"github.com/box1o/woki/internal/infrastructure/db/postgres"
	"github.com/box1o/woki/internal/infrastructure/file"
)

func (a *Application) setupRepositories() error {
	if a.cfg.Storage.Backend == "postgres" {
		a.userRepo = postgres.NewUserRepository(a.db.DB)
		a.workspaceRepo = postgres.NewWorkspaceRepository(a.db.DB)
		a.credentialRepo = postgres.NewCredentialRepository(a.db.DB)
		return nil
	}
	store, err := file.Open(a.cfg.Storage.DataFile)
	if err != nil {
		return err
	}
	a.fileStore = store
	a.userRepo = store.Users()
	a.workspaceRepo = store.Workspaces()
	a.credentialRepo = store.Credentials()
	return nil
}
