package redis

import (
	"context"
	"encoding/json"
	"fmt"
	"strings"
	"time"

	"github.com/box1o/woki/pkg/config"
	"github.com/box1o/woki/pkg/log"
	goredis "github.com/redis/go-redis/v9"
)

type Client struct {
	client *goredis.Client
	prefix string
}

var compareAndDeleteScript = goredis.NewScript(`
if redis.call("GET", KEYS[1]) == ARGV[1] then
	return redis.call("DEL", KEYS[1])
end
return 0
`)

func Open(cfg config.RedisConfig) (*Client, error) {
	options := &goredis.Options{
		Addr:        fmt.Sprintf("%s:%d", cfg.Host, cfg.Port),
		Username:    cfg.Username,
		Password:    cfg.Password,
		DB:          cfg.DB,
		DialTimeout: cfg.DialTimeout,
	}
	c := goredis.NewClient(options)
	ctx, cancel := context.WithTimeout(context.Background(), cfg.DialTimeout)
	defer cancel()
	if err := c.Ping(ctx).Err(); err != nil {
		_ = c.Close()
		return nil, ErrConnection.WithErr(err)
	}
	log.Info("Redis connected at %s:%d db=%d", cfg.Host, cfg.Port, cfg.DB)
	return &Client{client: c, prefix: strings.Trim(strings.TrimSpace(cfg.Prefix), ":")}, nil
}

func (c *Client) Key(parts ...string) string {
	clean := make([]string, 0, len(parts)+1)
	if c.prefix != "" {
		clean = append(clean, c.prefix)
	}
	for _, part := range parts {
		if part = strings.Trim(strings.TrimSpace(part), ":"); part != "" {
			clean = append(clean, part)
		}
	}
	return strings.Join(clean, ":")
}
func (c *Client) Get(ctx context.Context, key string) (string, error) {
	v, err := c.client.Get(ctx, key).Result()
	if err == goredis.Nil {
		return "", ErrNotFound.WithDetail(key)
	}
	if err != nil {
		return "", ErrOperation.WithErr(err)
	}
	return v, nil
}
func (c *Client) Set(ctx context.Context, key string, value any, ttl time.Duration) error {
	if err := c.client.Set(ctx, key, value, ttl).Err(); err != nil {
		return ErrOperation.WithErr(err)
	}
	return nil
}
func (c *Client) SetNX(ctx context.Context, key string, value any, ttl time.Duration) (bool, error) {
	ok, err := c.client.SetNX(ctx, key, value, ttl).Result()
	if err != nil {
		return false, ErrOperation.WithErr(err)
	}
	return ok, nil
}
func (c *Client) CompareAndDelete(ctx context.Context, key, expected string) (bool, error) {
	result, err := compareAndDeleteScript.Run(ctx, c.client, []string{key}, expected).Int64()
	if err != nil {
		return false, ErrOperation.WithErr(err)
	}
	return result == 1, nil
}
func (c *Client) Delete(ctx context.Context, keys ...string) error {
	if len(keys) == 0 {
		return nil
	}
	if err := c.client.Del(ctx, keys...).Err(); err != nil {
		return ErrOperation.WithErr(err)
	}
	return nil
}
func (c *Client) Exists(ctx context.Context, key string) (bool, error) {
	v, err := c.client.Exists(ctx, key).Result()
	if err != nil {
		return false, ErrOperation.WithErr(err)
	}
	return v > 0, nil
}
func (c *Client) Expire(ctx context.Context, key string, ttl time.Duration) error {
	if err := c.client.Expire(ctx, key, ttl).Err(); err != nil {
		return ErrOperation.WithErr(err)
	}
	return nil
}
func (c *Client) JSONSet(ctx context.Context, key string, value any, ttl time.Duration) error {
	b, err := json.Marshal(value)
	if err != nil {
		return ErrOperation.WithErr(err)
	}
	return c.Set(ctx, key, b, ttl)
}
func (c *Client) JSONGet(ctx context.Context, key string, dst any) error {
	value, err := c.Get(ctx, key)
	if err != nil {
		return err
	}
	if err := json.Unmarshal([]byte(value), dst); err != nil {
		return ErrOperation.WithErr(err)
	}
	return nil
}
func (c *Client) Ping(ctx context.Context) error {
	if err := c.client.Ping(ctx).Err(); err != nil {
		return ErrConnection.WithErr(err)
	}
	return nil
}
func (c *Client) Raw() *goredis.Client { return c.client }
func (c *Client) Close() error {
	if err := c.client.Close(); err != nil {
		return ErrConnection.WithErr(err)
	}
	return nil
}
func (c *Client) Shutdown(context.Context) error { return c.Close() }
