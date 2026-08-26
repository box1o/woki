package config

import (
	"net/url"
	"os"
	"strings"
)

type Config struct{ APIURL string }

func Load(value string) (Config, error) {
	if strings.TrimSpace(value) == "" {
		value = os.Getenv("WOKI_API_URL")
	}
	if strings.TrimSpace(value) == "" {
		value = "http://localhost:8080"
	}
	u, err := url.Parse(strings.TrimRight(strings.TrimSpace(value), "/"))
	if err != nil || u.Scheme == "" || u.Host == "" || (u.Scheme != "http" && u.Scheme != "https") {
		return Config{}, ErrAPIURLInvalid.WithDetail(value)
	}
	return Config{APIURL: u.String()}, nil
}
