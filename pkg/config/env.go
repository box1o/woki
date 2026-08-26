package config

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
)

func env(name, fallback string) string {
	if v := strings.TrimSpace(os.Getenv(name)); v != "" {
		return v
	}
	return fallback
}
func envBool(name string, fallback bool) (bool, error) {
	v := strings.TrimSpace(os.Getenv(name))
	if v == "" {
		return fallback, nil
	}
	x, err := strconv.ParseBool(v)
	if err != nil {
		return false, ErrValueParse.WithDetail(name).WithErr(err)
	}
	return x, nil
}
func envInt(name string, fallback int) (int, error) {
	v := strings.TrimSpace(os.Getenv(name))
	if v == "" {
		return fallback, nil
	}
	x, err := strconv.Atoi(v)
	if err != nil {
		return 0, ErrValueParse.WithDetail(name).WithErr(err)
	}
	return x, nil
}
func envDuration(name string, fallback time.Duration) (time.Duration, error) {
	v := strings.TrimSpace(os.Getenv(name))
	if v == "" {
		return fallback, nil
	}
	x, err := time.ParseDuration(v)
	if err != nil {
		return 0, ErrValueParse.WithDetail(name).WithErr(err)
	}
	return x, nil
}
func trimTrailingSlash(value string) string     { return strings.TrimRight(strings.TrimSpace(value), "/") }
func invalid(name, detail string) error         { return ErrInvalid.WithDetail(name + ": " + detail) }
func invalidCause(name string, err error) error { return ErrInvalid.WithDetail(name).WithErr(err) }

func loadEnvFiles(paths ...string) error {
	for _, path := range paths {
		file, err := os.Open(path)
		if os.IsNotExist(err) {
			continue
		}
		if err != nil {
			return ErrEnvFile.WithDetail(path).WithErr(err)
		}
		err = loadEnv(file)
		closeErr := file.Close()
		if err != nil {
			return ErrEnvFile.WithDetail(path).WithErr(err)
		}
		if closeErr != nil {
			return ErrEnvFile.WithDetail(path).WithErr(closeErr)
		}
		return nil
	}
	return nil
}
func loadEnv(file *os.File) error {
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		line = strings.TrimSpace(strings.TrimPrefix(line, "export "))
		key, value, ok := strings.Cut(line, "=")
		if !ok {
			return fmt.Errorf("invalid line %q", line)
		}
		key = strings.TrimSpace(key)
		value = strings.TrimSpace(value)
		if key == "" {
			return fmt.Errorf("empty environment key")
		}
		if len(value) >= 2 && ((value[0] == '"' && value[len(value)-1] == '"') || (value[0] == '\'' && value[len(value)-1] == '\'')) {
			value = value[1 : len(value)-1]
		}
		if _, exists := os.LookupEnv(key); !exists {
			if err := os.Setenv(key, value); err != nil {
				return err
			}
		}
	}
	return scanner.Err()
}
