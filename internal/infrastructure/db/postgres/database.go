package postgres

import (
	"context"
	"fmt"
	"time"

	"github.com/box1o/woki/pkg/config"
	"github.com/box1o/woki/pkg/log"
	gormdriver "gorm.io/driver/postgres"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"
)

type Database struct{ DB *gorm.DB }

func Open(cfg config.DatabaseConfig, environment config.Environment) (*Database, error) {
	dsn := fmt.Sprintf("host=%s port=%d user=%s password=%s dbname=%s sslmode=%s TimeZone=UTC",
		cfg.Host, cfg.Port, cfg.User, cfg.Password, cfg.Name, cfg.SSLMode)
	gormLog := logger.Silent
	if environment == config.Development {
		gormLog = logger.Error
	}
	db, err := gorm.Open(gormdriver.Open(dsn), &gorm.Config{
		Logger:         logger.Default.LogMode(gormLog),
		TranslateError: true,
		NowFunc:        func() time.Time { return time.Now().UTC() },
	})
	if err != nil {
		return nil, ErrConnection.WithErr(err)
	}
	sqlDB, err := db.DB()
	if err != nil {
		return nil, ErrConnection.WithErr(err)
	}
	sqlDB.SetMaxIdleConns(cfg.MaxIdleConns)
	sqlDB.SetMaxOpenConns(cfg.MaxOpenConns)
	sqlDB.SetConnMaxLifetime(cfg.ConnMaxLifetime)
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	if err := sqlDB.PingContext(ctx); err != nil {
		_ = sqlDB.Close()
		return nil, ErrConnection.WithErr(err)
	}
	log.Info("PostgreSQL connected at %s:%d/%s", cfg.Host, cfg.Port, cfg.Name)
	return &Database{DB: db}, nil
}

func (d *Database) Migrate() error {
	if err := d.DB.AutoMigrate(&DBUser{}, &DBWorkspace{}, &DBMember{}, &DBCredential{}); err != nil {
		return ErrMigration.WithErr(err)
	}

	indexes := []string{
		`CREATE UNIQUE INDEX IF NOT EXISTS idx_users_email_ci ON users (lower(email))`,
		`CREATE INDEX IF NOT EXISTS idx_users_email_search ON users (lower(email) text_pattern_ops)`,
		`CREATE INDEX IF NOT EXISTS idx_users_name_search ON users (lower(name) text_pattern_ops)`,
		`CREATE UNIQUE INDEX IF NOT EXISTS idx_workspace_owner_name_ci ON workspaces (owner_id, lower(name))`,
	}
	for _, statement := range indexes {
		if err := d.DB.Exec(statement).Error; err != nil {
			return ErrMigration.WithErr(err)
		}
	}
	return nil
}
func (d *Database) Ping(ctx context.Context) error {
	sqlDB, err := d.DB.DB()
	if err != nil {
		return ErrConnection.WithErr(err)
	}
	if err := sqlDB.PingContext(ctx); err != nil {
		return ErrConnection.WithErr(err)
	}
	return nil
}

func (d *Database) Close() error {
	sqlDB, err := d.DB.DB()
	if err != nil {
		return ErrConnection.WithErr(err)
	}
	if err := sqlDB.Close(); err != nil {
		return ErrConnection.WithErr(err)
	}
	return nil
}
func (d *Database) Shutdown(context.Context) error { return d.Close() }
