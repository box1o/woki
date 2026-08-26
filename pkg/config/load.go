package config

import (
	"os"
	"strings"
	"time"

	wokilog "github.com/box1o/woki/pkg/log"
)

func Load() (Config, error) {
	if err := loadEnvFiles(".env.local", ".env"); err != nil {
		return Config{}, err
	}
	level, err := wokilog.ParseLevel(env("WOKI_LOG_LEVEL", "info"))
	if err != nil {
		return Config{}, ErrLogConfig.WithDetail("WOKI_LOG_LEVEL").WithErr(err)
	}
	output, err := wokilog.ParseOutput(env("WOKI_LOG_OUTPUT", "console"))
	if err != nil {
		return Config{}, ErrLogConfig.WithDetail("WOKI_LOG_OUTPUT").WithErr(err)
	}

	cfg := Config{
		Environment: Environment(strings.ToLower(env("WOKI_ENV", string(Development)))),
		Version:     env("WOKI_VERSION", "dev"),
		Server: ServerConfig{
			Addr:              env("WOKI_ADDR", ":8080"),
			ReadHeaderTimeout: 5 * time.Second,
			ReadTimeout:       15 * time.Second,
			WriteTimeout:      30 * time.Second,
			IdleTimeout:       120 * time.Second,
			MaxHeaderBytes:    1 << 20,
		},
		Frontend: FrontendConfig{URL: trimTrailingSlash(env("WOKI_FRONTEND_URL", "http://localhost:5173"))},
		CORS:     CORSConfig{AllowedOrigin: trimTrailingSlash(env("WOKI_ALLOWED_ORIGIN", "http://localhost:5173"))},
		Database: DatabaseConfig{
			Host:            env("WOKI_DB_HOST", "127.0.0.1"),
			User:            env("WOKI_DB_USER", "woki"),
			Password:        os.Getenv("WOKI_DB_PASSWORD"),
			Name:            env("WOKI_DB_NAME", "woki"),
			SSLMode:         env("WOKI_DB_SSLMODE", "disable"),
			MaxIdleConns:    10,
			MaxOpenConns:    50,
			ConnMaxLifetime: time.Hour,
		},
		Redis: RedisConfig{
			Host:         env("WOKI_REDIS_HOST", "127.0.0.1"),
			Username:     strings.TrimSpace(os.Getenv("WOKI_REDIS_USERNAME")),
			Password:     os.Getenv("WOKI_REDIS_PASSWORD"),
			Prefix:       env("WOKI_REDIS_PREFIX", "woki"),
			DialTimeout:  5 * time.Second,
			ReadTimeout:  3 * time.Second,
			WriteTimeout: 3 * time.Second,
			PoolTimeout:  4 * time.Second,
			MaxRetries:   1,
		},
		Auth: AuthConfig{
			Google: GoogleConfig{
				ClientID:     strings.TrimSpace(os.Getenv("WOKI_GOOGLE_CLIENT_ID")),
				ClientSecret: strings.TrimSpace(os.Getenv("WOKI_GOOGLE_CLIENT_SECRET")),
				CallbackURL:  trimTrailingSlash(env("WOKI_GOOGLE_CALLBACK_URL", "http://localhost:8080/auth/google/callback")),
			},
			GitHub: GitHubConfig{
				ClientID:     strings.TrimSpace(os.Getenv("WOKI_GITHUB_CLIENT_ID")),
				ClientSecret: strings.TrimSpace(os.Getenv("WOKI_GITHUB_CLIENT_SECRET")),
			},
			Cookie: CookieConfig{Name: env("WOKI_SESSION_COOKIE", "woki_session"), SameSite: env("WOKI_COOKIE_SAME_SITE", "strict")},
		},
		Session:    SessionConfig{RedisPrefix: env("WOKI_SESSION_REDIS_PREFIX", "session")},
		DeviceAuth: DeviceAuthConfig{RedisPrefix: env("WOKI_DEVICE_REDIS_PREFIX", "device")},
		RateLimit: RateLimitConfig{
			Prefix:           env("WOKI_RATE_LIMIT_PREFIX", env("WOKI_REDIS_PREFIX", "woki")+":rate"),
			APILimit:         600,
			APIBurst:         60,
			APIWindow:        time.Minute,
			AuthLimit:        30,
			AuthBurst:        5,
			AuthWindow:       10 * time.Minute,
			DeviceCodeLimit:  10,
			DeviceCodeBurst:  3,
			DeviceCodeWindow: 10 * time.Minute,
			DevicePollLimit:  60,
			DevicePollBurst:  10,
			DevicePollWindow: time.Minute,
			MailLimit:        5,
			MailBurst:        2,
			MailWindow:       time.Hour,
		},
		Storage: StorageConfig{Backend: strings.ToLower(env("WOKI_STORAGE_BACKEND", "postgres")), DataFile: env("WOKI_DATA_FILE", "data/woki.json")},
		Mail: MailConfig{
			Host:      env("WOKI_MAIL_HOST", "smtp.gmail.com"),
			From:      env("WOKI_MAIL_FROM", "support@woki.sh"),
			Password:  os.Getenv("WOKI_MAIL_PASSWORD"),
			Name:      env("WOKI_MAIL_NAME", "Woki"),
			SupportTo: env("WOKI_MAIL_SUPPORT_TO", "support@woki.sh"),
		},
		Log: LogConfig{Output: output, Level: level, FilePath: env("WOKI_LOG_FILE", "data/woki.log"), DisableColors: os.Getenv("NO_COLOR") != ""},
	}

	bools := []struct {
		name     string
		dst      *bool
		fallback bool
	}{
		{"WOKI_DEBUG", &cfg.Debug, false},
		{"WOKI_DB_ENABLED", &cfg.Database.Enabled, true},
		{"WOKI_DB_MIGRATE", &cfg.Database.Migrate, true},
		{"WOKI_REDIS_ENABLED", &cfg.Redis.Enabled, true},
		{"WOKI_DEV_AUTH", &cfg.Auth.Dev, false},
		{"WOKI_COOKIE_SECURE", &cfg.Auth.Cookie.Secure, false},
		{"WOKI_RATE_LIMIT_ENABLED", &cfg.RateLimit.Enabled, true},
		{"WOKI_MAIL_ENABLED", &cfg.Mail.Enabled, false},
	}
	for _, item := range bools {
		v, err := envBool(item.name, item.fallback)
		if err != nil {
			return Config{}, err
		}
		*item.dst = v
	}

	ints := []struct {
		name     string
		dst      *int
		fallback int
	}{
		{"WOKI_DB_PORT", &cfg.Database.Port, 5432},
		{"WOKI_DB_MAX_IDLE_CONNS", &cfg.Database.MaxIdleConns, 10},
		{"WOKI_DB_MAX_OPEN_CONNS", &cfg.Database.MaxOpenConns, 50},
		{"WOKI_REDIS_PORT", &cfg.Redis.Port, 6379},
		{"WOKI_REDIS_DB", &cfg.Redis.DB, 0},
		{"WOKI_REDIS_MAX_RETRIES", &cfg.Redis.MaxRetries, 1},
		{"WOKI_RATE_LIMIT_API", &cfg.RateLimit.APILimit, 600},
		{"WOKI_RATE_LIMIT_API_BURST", &cfg.RateLimit.APIBurst, 60},
		{"WOKI_RATE_LIMIT_AUTH", &cfg.RateLimit.AuthLimit, 30},
		{"WOKI_RATE_LIMIT_AUTH_BURST", &cfg.RateLimit.AuthBurst, 5},
		{"WOKI_RATE_LIMIT_DEVICE_CODE", &cfg.RateLimit.DeviceCodeLimit, 10},
		{"WOKI_RATE_LIMIT_DEVICE_CODE_BURST", &cfg.RateLimit.DeviceCodeBurst, 3},
		{"WOKI_RATE_LIMIT_DEVICE_POLL", &cfg.RateLimit.DevicePollLimit, 60},
		{"WOKI_RATE_LIMIT_DEVICE_POLL_BURST", &cfg.RateLimit.DevicePollBurst, 10},
		{"WOKI_RATE_LIMIT_MAIL", &cfg.RateLimit.MailLimit, 5},
		{"WOKI_RATE_LIMIT_MAIL_BURST", &cfg.RateLimit.MailBurst, 2},
		{"WOKI_MAIL_PORT", &cfg.Mail.Port, 587},
		{"WOKI_MAIL_QUEUE_SIZE", &cfg.Mail.QueueSize, 128},
		{"WOKI_MAIL_WORKERS", &cfg.Mail.Workers, 2},
	}
	for _, item := range ints {
		v, err := envInt(item.name, item.fallback)
		if err != nil {
			return Config{}, err
		}
		*item.dst = v
	}

	durations := []struct {
		name     string
		dst      *time.Duration
		fallback time.Duration
	}{
		{"WOKI_SESSION_TTL", &cfg.Session.TTL, 24 * time.Hour},
		{"WOKI_DEVICE_CODE_TTL", &cfg.DeviceAuth.CodeTTL, 10 * time.Minute},
		{"WOKI_CLI_TOKEN_TTL", &cfg.DeviceAuth.CredentialTTL, 30 * 24 * time.Hour},
		{"WOKI_DB_CONN_MAX_LIFETIME", &cfg.Database.ConnMaxLifetime, time.Hour},
		{"WOKI_REDIS_DIAL_TIMEOUT", &cfg.Redis.DialTimeout, 5 * time.Second},
		{"WOKI_REDIS_READ_TIMEOUT", &cfg.Redis.ReadTimeout, 3 * time.Second},
		{"WOKI_REDIS_WRITE_TIMEOUT", &cfg.Redis.WriteTimeout, 3 * time.Second},
		{"WOKI_REDIS_POOL_TIMEOUT", &cfg.Redis.PoolTimeout, 4 * time.Second},
		{"WOKI_RATE_LIMIT_API_WINDOW", &cfg.RateLimit.APIWindow, time.Minute},
		{"WOKI_RATE_LIMIT_AUTH_WINDOW", &cfg.RateLimit.AuthWindow, 10 * time.Minute},
		{"WOKI_RATE_LIMIT_DEVICE_CODE_WINDOW", &cfg.RateLimit.DeviceCodeWindow, 10 * time.Minute},
		{"WOKI_RATE_LIMIT_DEVICE_POLL_WINDOW", &cfg.RateLimit.DevicePollWindow, time.Minute},
		{"WOKI_RATE_LIMIT_MAIL_WINDOW", &cfg.RateLimit.MailWindow, time.Hour},
		{"WOKI_MAIL_SEND_TIMEOUT", &cfg.Mail.SendTimeout, 15 * time.Second},
		{"WOKI_SHUTDOWN_TIMEOUT", &cfg.Shutdown.Timeout, 10 * time.Second},
	}
	for _, item := range durations {
		v, err := envDuration(item.name, item.fallback)
		if err != nil {
			return Config{}, err
		}
		*item.dst = v
	}

	if err := cfg.Validate(); err != nil {
		return Config{}, err
	}
	return cfg, nil
}
