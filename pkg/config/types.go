package config

import (
	"time"

	wokilog "github.com/box1o/woki/pkg/log"
)

type Environment string

const (
	Development Environment = "development"
	Staging     Environment = "staging"
	Production  Environment = "production"
)

type Config struct {
	Environment Environment
	Debug       bool
	Version     string
	Server      ServerConfig
	Frontend    FrontendConfig
	CORS        CORSConfig
	Database    DatabaseConfig
	Redis       RedisConfig
	Auth        AuthConfig
	Session     SessionConfig
	DeviceAuth  DeviceAuthConfig
	RateLimit   RateLimitConfig
	Storage     StorageConfig
	Mail        MailConfig
	Log         LogConfig
	Shutdown    ShutdownConfig
}

type ServerConfig struct {
	Addr              string
	ReadHeaderTimeout time.Duration
	ReadTimeout       time.Duration
	WriteTimeout      time.Duration
	IdleTimeout       time.Duration
	MaxHeaderBytes    int
}

type FrontendConfig struct{ URL string }
type CORSConfig struct{ AllowedOrigin string }

type DatabaseConfig struct {
	Enabled         bool
	Host            string
	Port            int
	User            string
	Password        string
	Name            string
	SSLMode         string
	Migrate         bool
	MaxIdleConns    int
	MaxOpenConns    int
	ConnMaxLifetime time.Duration
}

type RedisConfig struct {
	Enabled      bool
	Host         string
	Port         int
	Username     string
	Password     string
	DB           int
	Prefix       string
	DialTimeout  time.Duration
	ReadTimeout  time.Duration
	WriteTimeout time.Duration
	PoolTimeout  time.Duration
	MaxRetries   int
}

type AuthConfig struct {
	Google GoogleConfig
	Dev    bool
	Cookie CookieConfig
}

type GoogleConfig struct {
	ClientID     string
	ClientSecret string
	CallbackURL  string
}

type CookieConfig struct {
	Name     string
	Secure   bool
	SameSite string
}

type SessionConfig struct {
	TTL         time.Duration
	RedisPrefix string
}

type DeviceAuthConfig struct {
	CodeTTL       time.Duration
	CredentialTTL time.Duration
	RedisPrefix   string
}

type RateLimitConfig struct {
	Enabled          bool
	Prefix           string
	APILimit         int
	APIBurst         int
	APIWindow        time.Duration
	AuthLimit        int
	AuthBurst        int
	AuthWindow       time.Duration
	DeviceCodeLimit  int
	DeviceCodeBurst  int
	DeviceCodeWindow time.Duration
	DevicePollLimit  int
	DevicePollBurst  int
	DevicePollWindow time.Duration
	MailLimit        int
	MailBurst        int
	MailWindow       time.Duration
}

type StorageConfig struct {
	Backend  string
	DataFile string
}

type MailConfig struct {
	Enabled     bool
	Host        string
	Port        int
	From        string
	Password    string
	Name        string
	SupportTo   string
	QueueSize   int
	Workers     int
	SendTimeout time.Duration
}

type LogConfig struct {
	Output        wokilog.OutputType
	Level         wokilog.LevelType
	FilePath      string
	DisableColors bool
}

type ShutdownConfig struct{ Timeout time.Duration }
