package config

import (
	"errors"
	"os"
	"path/filepath"
	"testing"
	"time"

	wokilog "github.com/box1o/woki/pkg/log"
)

func validConfig() Config {
	return Config{
		Environment: Development,
		Version:     "test",
		Server: ServerConfig{
			Addr:              ":8080",
			ReadHeaderTimeout: time.Second,
			ReadTimeout:       time.Second,
			WriteTimeout:      time.Second,
			IdleTimeout:       time.Second,
			MaxHeaderBytes:    1 << 20,
		},
		Frontend:   FrontendConfig{URL: "http://localhost:5173"},
		CORS:       CORSConfig{AllowedOrigin: "http://localhost:5173"},
		Auth:       AuthConfig{Dev: true, Cookie: CookieConfig{Name: "woki_session", SameSite: "strict"}},
		Session:    SessionConfig{TTL: time.Hour},
		DeviceAuth: DeviceAuthConfig{CodeTTL: time.Minute, CredentialTTL: time.Hour},
		Storage:    StorageConfig{Backend: "file", DataFile: "data/woki.json"},
		Log:        LogConfig{Output: wokilog.Console, Level: wokilog.InfoLevel},
		Shutdown:   ShutdownConfig{Timeout: time.Second},
	}
}

func TestValidate(t *testing.T) {
	cfg := validConfig()
	if err := cfg.Validate(); err != nil {
		t.Fatal(err)
	}
	cfg.Server.Addr = "bad"
	if err := cfg.Validate(); !errors.Is(err, ErrInvalid) {
		t.Fatalf("Validate()=%v; want ErrInvalid", err)
	}
}

func TestProductionRejectsDevelopmentAuthentication(t *testing.T) {
	cfg := validConfig()
	cfg.Environment = Production
	cfg.Auth.Dev = true
	cfg.Auth.Cookie.Secure = true
	if err := cfg.Validate(); !errors.Is(err, ErrInvalid) {
		t.Fatalf("Validate()=%v; want ErrInvalid", err)
	}
}

func TestLoadEnvFileDoesNotOverrideProcessEnvironment(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, ".env")
	if err := os.WriteFile(path, []byte("WOKI_VERSION=file\nWOKI_ADDR=:9191\n"), 0o600); err != nil {
		t.Fatal(err)
	}
	t.Setenv("WOKI_VERSION", "process")
	if err := loadEnvFiles(path); err != nil {
		t.Fatal(err)
	}
	if got := os.Getenv("WOKI_VERSION"); got != "process" {
		t.Fatalf("WOKI_VERSION=%q", got)
	}
	if got := os.Getenv("WOKI_ADDR"); got != ":9191" {
		t.Fatalf("WOKI_ADDR=%q", got)
	}
}

func TestValidateRejectsInvalidLogOutput(t *testing.T) {
	cfg := validConfig()
	cfg.Log.Output = wokilog.OutputType(99)
	if err := cfg.Validate(); !errors.Is(err, ErrLogConfig) {
		t.Fatalf("Validate()=%v; want ErrLogConfig", err)
	}
}

func TestValidateRejectsInvalidRedisTimeout(t *testing.T) {
	cfg := validConfig()
	cfg.Redis = RedisConfig{
		Enabled:      true,
		Host:         "127.0.0.1",
		Port:         6379,
		Prefix:       "woki",
		DialTimeout:  time.Second,
		ReadTimeout:  0,
		WriteTimeout: time.Second,
		PoolTimeout:  time.Second,
	}
	if err := cfg.Validate(); !errors.Is(err, ErrInvalid) {
		t.Fatalf("Validate()=%v; want ErrInvalid", err)
	}
}
