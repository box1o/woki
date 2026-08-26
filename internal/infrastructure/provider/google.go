package provider

import (
	"context"
	"encoding/json"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/config"
)

type Google struct {
	cfg         config.GoogleConfig
	client      *http.Client
	tokenURL    string
	userInfoURL string
}

func NewGoogle(cfg config.GoogleConfig) *Google {
	return &Google{
		cfg:         cfg,
		client:      &http.Client{Timeout: 10 * time.Second},
		tokenURL:    "https://oauth2.googleapis.com/token",
		userInfoURL: "https://openidconnect.googleapis.com/v1/userinfo",
	}
}

func (g *Google) Configured() bool {
	return g.cfg.ClientID != "" && g.cfg.ClientSecret != "" && g.cfg.CallbackURL != ""
}

func (g *Google) AuthURL(state string) string {
	values := url.Values{
		"client_id":     {g.cfg.ClientID},
		"redirect_uri":  {g.cfg.CallbackURL},
		"response_type": {"code"},
		"scope":         {"openid email profile"},
		"state":         {state},
		"access_type":   {"online"},
		"prompt":        {"select_account"},
	}
	return "https://accounts.google.com/o/oauth2/v2/auth?" + values.Encode()
}

func (g *Google) Exchange(ctx context.Context, code string) (Profile, error) {
	form := url.Values{
		"client_id":     {g.cfg.ClientID},
		"client_secret": {g.cfg.ClientSecret},
		"redirect_uri":  {g.cfg.CallbackURL},
		"grant_type":    {"authorization_code"},
		"code":          {strings.TrimSpace(code)},
	}
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, g.tokenURL, strings.NewReader(form.Encode()))
	if err != nil {
		return Profile{}, ErrRequestFailed.WithErr(err)
	}
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	resp, err := g.client.Do(req)
	if err != nil {
		return Profile{}, ErrTokenExchange.WithErr(err)
	}
	defer resp.Body.Close()
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		_, _ = io.Copy(io.Discard, io.LimitReader(resp.Body, 8<<10))
		return Profile{}, ErrTokenExchange.WithDetail(resp.Status)
	}
	var token struct {
		AccessToken      string `json:"access_token"`
		TokenType        string `json:"token_type"`
		Error            string `json:"error"`
		ErrorDescription string `json:"error_description"`
	}
	if err := json.NewDecoder(io.LimitReader(resp.Body, 1<<20)).Decode(&token); err != nil {
		return Profile{}, ErrResponse.WithErr(err)
	}
	if token.Error != "" {
		return Profile{}, ErrOAuthDenied.WithDetail(strings.TrimSpace(token.Error + ": " + token.ErrorDescription))
	}
	if token.AccessToken == "" {
		return Profile{}, ErrResponse.WithDetail("access token is empty")
	}

	req, err = http.NewRequestWithContext(ctx, http.MethodGet, g.userInfoURL, nil)
	if err != nil {
		return Profile{}, ErrRequestFailed.WithErr(err)
	}
	req.Header.Set("Authorization", "Bearer "+token.AccessToken)
	resp, err = g.client.Do(req)
	if err != nil {
		return Profile{}, ErrRequestFailed.WithErr(err)
	}
	defer resp.Body.Close()
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		_, _ = io.Copy(io.Discard, io.LimitReader(resp.Body, 8<<10))
		return Profile{}, ErrProfile.WithDetail(resp.Status)
	}
	var info struct {
		Subject       string `json:"sub"`
		Email         string `json:"email"`
		EmailVerified bool   `json:"email_verified"`
		Name          string `json:"name"`
		Picture       string `json:"picture"`
	}
	if err := json.NewDecoder(io.LimitReader(resp.Body, 1<<20)).Decode(&info); err != nil {
		return Profile{}, ErrResponse.WithErr(err)
	}
	if info.Subject == "" || info.Email == "" || !info.EmailVerified {
		return Profile{}, ErrProfile.WithDetail("verified Google identity is unavailable")
	}
	name := strings.TrimSpace(info.Name)
	if name == "" {
		name = strings.Split(info.Email, "@")[0]
	}
	return Profile{
		Email:      info.Email,
		Name:       name,
		AvatarURL:  info.Picture,
		Provider:   user.ProviderGoogle,
		ProviderID: info.Subject,
	}, nil
}
