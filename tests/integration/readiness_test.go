package integration_test

import (
	"net/http"
	"net/http/httptest"
	"os"
	"testing"
	"time"

	"github.com/box1o/woki/internal/application"
	"github.com/box1o/woki/pkg/config"
	wokilog "github.com/box1o/woki/pkg/log"
)

func TestPostgresRedisReadiness(t *testing.T) {
	if os.Getenv("WOKI_TEST_INTEGRATION") != "1" {
		t.Skip("set WOKI_TEST_INTEGRATION=1 to run against PostgreSQL and Redis")
	}

	cfg := config.Config{
		Environment: config.Development,
		Version:     "integration-test",
		Server: config.ServerConfig{
			Addr:              ":0",
			ReadHeaderTimeout: time.Second,
			ReadTimeout:       5 * time.Second,
			WriteTimeout:      5 * time.Second,
			IdleTimeout:       5 * time.Second,
			MaxHeaderBytes:    1 << 20,
		},
		Frontend: config.FrontendConfig{URL: "http://localhost:5173"},
		CORS:     config.CORSConfig{AllowedOrigin: "http://localhost:5173"},
		Database: config.DatabaseConfig{
			Enabled:         true,
			Host:            env("WOKI_TEST_DB_HOST", "127.0.0.1"),
			Port:            5432,
			User:            env("WOKI_TEST_DB_USER", "woki"),
			Password:        env("WOKI_TEST_DB_PASSWORD", "woki"),
			Name:            env("WOKI_TEST_DB_NAME", "woki"),
			SSLMode:         "disable",
			Migrate:         true,
			MaxIdleConns:    1,
			MaxOpenConns:    2,
			ConnMaxLifetime: time.Minute,
		},
		Redis: config.RedisConfig{
			Enabled:      true,
			Host:         env("WOKI_TEST_REDIS_HOST", "127.0.0.1"),
			Port:         6379,
			Prefix:       "woki:test:integration",
			DialTimeout:  time.Second,
			ReadTimeout:  time.Second,
			WriteTimeout: time.Second,
			PoolTimeout:  time.Second,
			MaxRetries:   0,
		},
		Auth:       config.AuthConfig{Dev: true, Cookie: config.CookieConfig{Name: "woki_test_session", SameSite: "strict"}},
		Session:    config.SessionConfig{TTL: time.Hour, RedisPrefix: "session"},
		DeviceAuth: config.DeviceAuthConfig{CodeTTL: time.Minute, CredentialTTL: time.Hour, RedisPrefix: "device"},
		RateLimit: config.RateLimitConfig{
			Enabled: false,
		},
		Storage:  config.StorageConfig{Backend: "postgres"},
		Log:      config.LogConfig{Output: wokilog.Console, Level: wokilog.ErrorLevel},
		Shutdown: config.ShutdownConfig{Timeout: time.Second},
	}

	app, err := application.New(cfg)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() {
		if err := app.Shutdown(t.Context()); err != nil {
			t.Errorf("Shutdown() error = %v", err)
		}
	})

	response := httptest.NewRecorder()
	request := httptest.NewRequest(http.MethodGet, "/ready", nil)
	app.Handler().ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("GET /ready status = %d; body = %s", response.Code, response.Body.String())
	}
}

func env(name, fallback string) string {
	if value := os.Getenv(name); value != "" {
		return value
	}
	return fallback
}
