// Package api provides the typed HTTP client used by the Woki CLI.
package api

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"

	contracts "github.com/box1o/woki/pkg/api"
)

type Error struct {
	Status  int
	Code    string
	Message string
	Detail  string
}

func (e *Error) Error() string {
	if e.Code != "" && e.Detail != "" {
		return fmt.Sprintf("%s: %s: %s", e.Code, e.Message, e.Detail)
	}
	if e.Code != "" {
		return fmt.Sprintf("%s: %s", e.Code, e.Message)
	}
	if e.Detail != "" {
		return fmt.Sprintf("%s: %s", e.Message, e.Detail)
	}
	return e.Message
}

type Client struct {
	baseURL    string
	token      string
	httpClient *http.Client
}

func New(baseURL string) (*Client, error) {
	u, err := url.Parse(strings.TrimRight(strings.TrimSpace(baseURL), "/"))
	if err != nil || u.Scheme == "" || u.Host == "" || (u.Scheme != "http" && u.Scheme != "https") {
		return nil, ErrURLInvalid.WithDetail(baseURL)
	}
	return &Client{baseURL: u.String(), httpClient: &http.Client{Timeout: 30 * time.Second}}, nil
}

// WithToken returns an authenticated copy. It never mutates the shared client.
func (c *Client) WithToken(token string) *Client {
	clone := *c
	clone.token = strings.TrimSpace(token)
	return &clone
}

func (c *Client) BaseURL() string { return c.baseURL }
func (c *Client) do(ctx context.Context, method, path string, body, out any) error {
	var reader io.Reader
	if body != nil {
		data, err := json.Marshal(body)
		if err != nil {
			return ErrRequestEncode.WithErr(err)
		}
		reader = bytes.NewReader(data)
	}
	req, err := http.NewRequestWithContext(ctx, method, c.baseURL+path, reader)
	if err != nil {
		return ErrRequestBuild.WithErr(err)
	}
	req.Header.Set("Accept", "application/json")
	req.Header.Set("User-Agent", "woki-cli/dev")
	if body != nil {
		req.Header.Set("Content-Type", "application/json")
	}
	if c.token != "" {
		req.Header.Set("Authorization", "Bearer "+c.token)
	}
	resp, err := c.httpClient.Do(req)
	if err != nil {
		return ErrRequestFailed.WithDetail(method + " " + path).WithErr(err)
	}
	defer resp.Body.Close()
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		return decodeError(resp)
	}
	if out == nil || resp.StatusCode == http.StatusNoContent {
		return nil
	}
	if err := json.NewDecoder(io.LimitReader(resp.Body, 2<<20)).Decode(out); err != nil {
		if errors.Is(err, io.EOF) {
			return nil
		}
		return ErrResponseDecode.WithErr(err)
	}
	return nil
}
func decodeError(resp *http.Response) error {
	var envelope contracts.ErrorResponse
	if err := json.NewDecoder(io.LimitReader(resp.Body, 64<<10)).Decode(&envelope); err == nil && envelope.Error.Message != "" {
		return &Error{Status: resp.StatusCode, Code: envelope.Error.Code, Message: envelope.Error.Message, Detail: envelope.Error.Detail}
	}
	return &Error{Status: resp.StatusCode, Code: "HTTP_ERROR", Message: resp.Status}
}

func (c *Client) CreateDeviceCode(ctx context.Context, clientName string) (contracts.DeviceCodeResponse, error) {
	var out contracts.DeviceCodeResponse
	err := c.do(ctx, http.MethodPost, "/auth/device/code", map[string]string{"client_name": clientName}, &out)
	return out, err
}
func (c *Client) ExchangeDeviceCode(ctx context.Context, deviceCode string) (contracts.DeviceTokenResponse, error) {
	var out contracts.DeviceTokenResponse
	err := c.do(ctx, http.MethodPost, "/auth/device/token", map[string]string{"device_code": deviceCode}, &out)
	return out, err
}
func (c *Client) CLIStatus(ctx context.Context) (map[string]any, error) {
	var out map[string]any
	err := c.do(ctx, http.MethodGet, "/auth/cli/status", nil, &out)
	return out, err
}
func (c *Client) CLILogout(ctx context.Context) error {
	return c.do(ctx, http.MethodPost, "/auth/cli/logout", nil, nil)
}
func (c *Client) ListWorkspaces(ctx context.Context) ([]contracts.Workspace, error) {
	var out []contracts.Workspace
	err := c.do(ctx, http.MethodGet, "/workspaces", nil, &out)
	return out, err
}
func (c *Client) CreateWorkspace(ctx context.Context, name string) (contracts.Workspace, error) {
	var out contracts.Workspace
	err := c.do(ctx, http.MethodPost, "/workspaces", map[string]string{"name": name}, &out)
	return out, err
}
func (c *Client) DeleteWorkspace(ctx context.Context, id string) error {
	return c.do(ctx, http.MethodDelete, "/workspaces/"+url.PathEscape(id), nil, nil)
}
func (c *Client) ListMembers(ctx context.Context, wid string) ([]contracts.Member, error) {
	var out []contracts.Member
	err := c.do(ctx, http.MethodGet, "/workspaces/"+url.PathEscape(wid)+"/members", nil, &out)
	return out, err
}
func (c *Client) MemberCandidates(ctx context.Context, wid, query string, limit int) ([]contracts.User, error) {
	if limit <= 0 {
		limit = 8
	}
	values := url.Values{}
	values.Set("q", strings.TrimSpace(query))
	values.Set("limit", fmt.Sprintf("%d", limit))
	var out []contracts.User
	err := c.do(ctx, http.MethodGet, "/workspaces/"+url.PathEscape(wid)+"/member-candidates?"+values.Encode(), nil, &out)
	return out, err
}
func (c *Client) AddMember(ctx context.Context, wid, email, role string) (contracts.Member, error) {
	var out contracts.Member
	err := c.do(ctx, http.MethodPost, "/workspaces/"+url.PathEscape(wid)+"/members", map[string]string{"email": email, "role": role}, &out)
	return out, err
}
func (c *Client) RemoveMember(ctx context.Context, wid, mid string) error {
	return c.do(ctx, http.MethodDelete, "/workspaces/"+url.PathEscape(wid)+"/members/"+url.PathEscape(mid), nil, nil)
}
func (c *Client) UpdateMemberRole(ctx context.Context, wid, mid, role string) (contracts.Member, error) {
	var out contracts.Member
	err := c.do(ctx, http.MethodPatch, "/workspaces/"+url.PathEscape(wid)+"/members/"+url.PathEscape(mid), map[string]string{"role": role}, &out)
	return out, err
}
