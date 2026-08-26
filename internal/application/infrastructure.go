package application

import (
	"github.com/box1o/woki/internal/infrastructure/db/postgres"
	eventinfra "github.com/box1o/woki/internal/infrastructure/events"
	infraratelimit "github.com/box1o/woki/internal/infrastructure/ratelimit"
	redisratelimit "github.com/box1o/woki/internal/infrastructure/ratelimit/redis"
	infraredis "github.com/box1o/woki/internal/infrastructure/redis"
	"github.com/box1o/woki/pkg/config"
	"github.com/box1o/woki/pkg/log"
)

func (a *Application) setupInfrastructure() error {
	a.eventBus = eventinfra.NewBus()
	if a.cfg.Database.Enabled && a.cfg.Storage.Backend == "postgres" {
		db, err := postgres.Open(a.cfg.Database, a.cfg.Environment)
		if err != nil {
			return err
		}
		a.db = db
		if a.cfg.Database.Migrate {
			if err := a.db.Migrate(); err != nil {
				return err
			}
		}
	}
	if a.cfg.Redis.Enabled {
		client, err := infraredis.Open(a.cfg.Redis)
		if err != nil {
			return err
		}
		a.redis = client
		var fallback *infraratelimit.Memory
		if a.cfg.Environment != config.Production {
			fallback = infraratelimit.NewMemory()
		}
		a.limiter = redisratelimit.New(client.Raw(), fallback)
	} else {
		log.Warn("Redis is disabled; using process-local sessions, device state, and rate limiting")
		a.limiter = infraratelimit.NewMemory()
	}
	return nil
}
