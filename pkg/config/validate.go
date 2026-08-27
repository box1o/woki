package config

import (
	"fmt"
	"net"
	"net/mail"
	"net/url"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	wokilog "github.com/box1o/woki/pkg/log"
)

func (c Config) Validate() error {
	if !c.Environment.Valid() {
		return invalid("WOKI_ENV", "must be development, staging, or production")
	}
	if strings.TrimSpace(c.Version) == "" {
		return invalid("WOKI_VERSION", "must not be empty")
	}
	if err := validateListenAddress(c.Server.Addr); err != nil {
		return invalidCause("WOKI_ADDR", err)
	}
	if err := validateHTTPURL(c.Frontend.URL, false); err != nil {
		return invalidCause("WOKI_FRONTEND_URL", err)
	}
	if err := validateHTTPURL(c.CORS.AllowedOrigin, true); err != nil {
		return invalidCause("WOKI_ALLOWED_ORIGIN", err)
	}
	if c.Storage.Backend != "postgres" && c.Storage.Backend != "file" {
		return invalid("WOKI_STORAGE_BACKEND", "must be postgres or file")
	}
	if !c.Log.Output.Valid() {
		return ErrLogConfig.WithDetail("WOKI_LOG_OUTPUT")
	}
	if !c.Log.Level.Valid() {
		return ErrLogConfig.WithDetail("WOKI_LOG_LEVEL")
	}
	if (c.Log.Output == wokilog.File || c.Log.Output == wokilog.ConsoleAndFile) && strings.TrimSpace(c.Log.FilePath) == "" {
		return ErrLogConfig.WithDetail("WOKI_LOG_FILE")
	}
	if c.Storage.Backend == "file" && (strings.TrimSpace(c.Storage.DataFile) == "" || filepath.Clean(c.Storage.DataFile) == ".") {
		return invalid("WOKI_DATA_FILE", "must name a file")
	}
	if c.Storage.Backend == "postgres" && !c.Database.Enabled {
		return invalid("WOKI_DB_ENABLED", "must be enabled when storage backend is postgres")
	}
	if c.Database.Enabled {
		if strings.TrimSpace(c.Database.Host) == "" || c.Database.Port < 1 || c.Database.Port > 65535 {
			return invalid("database", "host and valid port are required")
		}
		if strings.TrimSpace(c.Database.User) == "" || strings.TrimSpace(c.Database.Name) == "" {
			return invalid("database", "user and database name are required")
		}
		if c.Database.MaxIdleConns < 0 || c.Database.MaxOpenConns < 1 || c.Database.MaxIdleConns > c.Database.MaxOpenConns {
			return invalid("database pool", "connection limits are invalid")
		}
		switch c.Database.SSLMode {
		case "disable", "allow", "prefer", "require", "verify-ca", "verify-full":
		default:
			return invalid("WOKI_DB_SSLMODE", "unsupported PostgreSQL SSL mode")
		}
	}
	if c.Redis.Enabled {
		if strings.TrimSpace(c.Redis.Host) == "" || c.Redis.Port < 1 || c.Redis.Port > 65535 {
			return invalid("redis", "host and valid port are required")
		}
		if strings.TrimSpace(c.Redis.Prefix) == "" {
			return invalid("WOKI_REDIS_PREFIX", "must not be empty")
		}
		if c.Redis.DB < 0 {
			return invalid("WOKI_REDIS_DB", "must not be negative")
		}
		for name, timeout := range map[string]time.Duration{
			"WOKI_REDIS_DIAL_TIMEOUT":  c.Redis.DialTimeout,
			"WOKI_REDIS_READ_TIMEOUT":  c.Redis.ReadTimeout,
			"WOKI_REDIS_WRITE_TIMEOUT": c.Redis.WriteTimeout,
			"WOKI_REDIS_POOL_TIMEOUT":  c.Redis.PoolTimeout,
		} {
			if err := validatePositiveDuration(name, timeout); err != nil {
				return err
			}
		}
		if c.Redis.MaxRetries < 0 || c.Redis.MaxRetries > 10 {
			return invalid("WOKI_REDIS_MAX_RETRIES", "must be between 0 and 10")
		}
	}
	if strings.TrimSpace(c.Auth.Cookie.Name) == "" || strings.ContainsAny(c.Auth.Cookie.Name, "\r\n;= \t") {
		return invalid("WOKI_SESSION_COOKIE", "invalid cookie name")
	}
	if c.Auth.Cookie.SameSite != "strict" && c.Auth.Cookie.SameSite != "lax" && c.Auth.Cookie.SameSite != "none" {
		return invalid("WOKI_COOKIE_SAME_SITE", "must be strict, lax, or none")
	}
	if c.Auth.Cookie.SameSite == "none" && !c.Auth.Cookie.Secure {
		return invalid("WOKI_COOKIE_SECURE", "must be enabled when SameSite is none")
	}
	if (c.Auth.Google.ClientID == "") != (c.Auth.Google.ClientSecret == "") {
		return invalid("Google OAuth", "client ID and client secret must be configured together")
	}
	if c.Auth.Google.ClientID != "" {
		if err := validateHTTPURL(c.Auth.Google.CallbackURL, false); err != nil {
			return invalidCause("WOKI_GOOGLE_CALLBACK_URL", err)
		}
	}
	if !c.Auth.Dev && c.Auth.Google.ClientID == "" {
		return ErrAuthRequired
	}
	if c.Mail.Enabled {
		if strings.TrimSpace(c.Mail.Host) == "" || c.Mail.Port < 1 || c.Mail.Port > 65535 {
			return invalid("mail", "SMTP host and port are required")
		}
		if strings.TrimSpace(c.Mail.From) == "" || c.Mail.Password == "" {
			return invalid("mail", "from address and password are required")
		}
		if _, err := mail.ParseAddress(c.Mail.From); err != nil {
			return invalidCause("WOKI_MAIL_FROM", err)
		}
		if strings.TrimSpace(c.Mail.SupportTo) == "" {
			return invalid("WOKI_MAIL_SUPPORT_TO", "support recipient is required when mail is enabled")
		}
		if _, err := mail.ParseAddress(c.Mail.SupportTo); err != nil {
			return invalidCause("WOKI_MAIL_SUPPORT_TO", err)
		}
		if c.Mail.QueueSize < 1 || c.Mail.QueueSize > 10000 {
			return invalid("WOKI_MAIL_QUEUE_SIZE", "must be between 1 and 10000")
		}
		if c.Mail.Workers < 1 || c.Mail.Workers > 32 {
			return invalid("WOKI_MAIL_WORKERS", "must be between 1 and 32")
		}
		if err := validatePositiveDuration("WOKI_MAIL_SEND_TIMEOUT", c.Mail.SendTimeout); err != nil {
			return err
		}
	}
	if c.RateLimit.Enabled {
		policies := []struct {
			limitName string
			burstName string
			limit     int
			burst     int
		}{
			{"WOKI_RATE_LIMIT_API", "WOKI_RATE_LIMIT_API_BURST", c.RateLimit.APILimit, c.RateLimit.APIBurst},
			{"WOKI_RATE_LIMIT_AUTH", "WOKI_RATE_LIMIT_AUTH_BURST", c.RateLimit.AuthLimit, c.RateLimit.AuthBurst},
			{"WOKI_RATE_LIMIT_DEVICE_CODE", "WOKI_RATE_LIMIT_DEVICE_CODE_BURST", c.RateLimit.DeviceCodeLimit, c.RateLimit.DeviceCodeBurst},
			{"WOKI_RATE_LIMIT_DEVICE_POLL", "WOKI_RATE_LIMIT_DEVICE_POLL_BURST", c.RateLimit.DevicePollLimit, c.RateLimit.DevicePollBurst},
			{"WOKI_RATE_LIMIT_MAIL", "WOKI_RATE_LIMIT_MAIL_BURST", c.RateLimit.MailLimit, c.RateLimit.MailBurst},
		}
		for _, policy := range policies {
			if policy.limit < 1 {
				return invalid(policy.limitName, "must be at least 1")
			}
			if policy.burst < 1 {
				return invalid(policy.burstName, "must be at least 1")
			}
		}
		windows := map[string]time.Duration{
			"WOKI_RATE_LIMIT_API_WINDOW":         c.RateLimit.APIWindow,
			"WOKI_RATE_LIMIT_AUTH_WINDOW":        c.RateLimit.AuthWindow,
			"WOKI_RATE_LIMIT_DEVICE_CODE_WINDOW": c.RateLimit.DeviceCodeWindow,
			"WOKI_RATE_LIMIT_DEVICE_POLL_WINDOW": c.RateLimit.DevicePollWindow,
			"WOKI_RATE_LIMIT_MAIL_WINDOW":        c.RateLimit.MailWindow,
		}
		for name, window := range windows {
			if err := validatePositiveDuration(name, window); err != nil {
				return err
			}
		}
	}
	for name, d := range map[string]time.Duration{
		"WOKI_SESSION_TTL": c.Session.TTL, "WOKI_DEVICE_CODE_TTL": c.DeviceAuth.CodeTTL,
		"WOKI_CLI_TOKEN_TTL": c.DeviceAuth.CredentialTTL, "WOKI_SHUTDOWN_TIMEOUT": c.Shutdown.Timeout,
	} {
		if err := validatePositiveDuration(name, d); err != nil {
			return err
		}
	}
	if c.Environment == Production {
		if c.Auth.Dev {
			return invalid("WOKI_DEV_AUTH", "must be disabled in production")
		}
		if !c.Auth.Cookie.Secure {
			return invalid("WOKI_COOKIE_SECURE", "must be enabled in production")
		}
		if c.Storage.Backend != "postgres" {
			return invalid("WOKI_STORAGE_BACKEND", "production must use postgres")
		}
		if !c.Redis.Enabled {
			return invalid("WOKI_REDIS_ENABLED", "production must use Redis")
		}
	}
	return nil
}

func (e Environment) Valid() bool {
	switch e {
	case Development, Staging, Production:
		return true
	}
	return false
}
func validatePositiveDuration(name string, value time.Duration) error {
	if value < time.Second {
		return invalid(name, "must be at least 1s")
	}
	return nil
}
func validateListenAddress(value string) error {
	value = strings.TrimSpace(value)
	if value == "" {
		return fmt.Errorf("must not be empty")
	}
	_, port, err := net.SplitHostPort(value)
	if err != nil {
		return fmt.Errorf("must be host:port: %w", err)
	}
	n, err := strconv.Atoi(port)
	if err != nil || n < 0 || n > 65535 {
		return fmt.Errorf("port must be between 0 and 65535")
	}
	return nil
}
func validateHTTPURL(value string, originOnly bool) error {
	u, err := url.Parse(strings.TrimSpace(value))
	if err != nil || u.Scheme == "" || u.Host == "" {
		return fmt.Errorf("must be an absolute HTTP URL")
	}
	if u.Scheme != "http" && u.Scheme != "https" {
		return fmt.Errorf("scheme must be http or https")
	}
	if u.User != nil || u.Fragment != "" {
		return fmt.Errorf("must not contain user info or fragment")
	}
	if originOnly && (u.Path != "" || u.RawQuery != "") {
		return fmt.Errorf("must contain only scheme and host")
	}
	return nil
}
