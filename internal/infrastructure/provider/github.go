// Package provider contains external identity-provider adapters.
package provider

import (
	"context"
	"encoding/json"
	"io"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"time"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/config"
)

type Profile struct {
	Email      string
	Name       string
	AvatarURL  string
	Provider   user.Provider
	ProviderID string
}

type GitHub struct {
	cfg      config.GitHubConfig
	client   *http.Client
	tokenURL string
	apiURL   string
}

func NewGitHub(cfg config.GitHubConfig) *GitHub {
	return &GitHub{
		cfg:      cfg,
		client:   &http.Client{Timeout: 10 * time.Second},
		tokenURL: "https://github.com/login/oauth/access_token",
		apiURL:   "https://api.github.com",
	}
}
func (g *GitHub) Configured() bool { return g.cfg.ClientID != "" && g.cfg.ClientSecret != "" }
func (g *GitHub) AuthURL(state string) string {
	v := url.Values{"client_id": {g.cfg.ClientID}, "state": {state}, "scope": {"read:user user:email"}}
	return "https://github.com/login/oauth/authorize?" + v.Encode()
}
func (g *GitHub) Exchange(ctx context.Context, code string) (Profile, error) {
	form := url.Values{"client_id": {g.cfg.ClientID}, "client_secret": {g.cfg.ClientSecret}, "code": {strings.TrimSpace(code)}}
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, g.tokenURL, strings.NewReader(form.Encode()))
	if err != nil {
		return Profile{}, ErrRequestFailed.WithErr(err)
	}
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	resp, err := g.client.Do(req)
	if err != nil {
		return Profile{}, ErrTokenExchange.WithErr(err)
	}
	defer resp.Body.Close()
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		_, _ = io.Copy(io.Discard, io.LimitReader(resp.Body, 4<<10))
		return Profile{}, ErrTokenExchange.WithDetail(resp.Status)
	}
	var token struct {
		AccessToken      string `json:"access_token"`
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
	profile, err := g.fetchProfile(ctx, token.AccessToken)
	if err != nil {
		return Profile{}, err
	}
	profile.Provider = user.ProviderGitHub
	return profile, nil
}
func (g *GitHub) fetchProfile(ctx context.Context, token string) (Profile, error) {
	var ghUser struct {
		ID        int64  `json:"id"`
		Login     string `json:"login"`
		Name      string `json:"name"`
		AvatarURL string `json:"avatar_url"`
	}
	if err := g.getJSON(ctx, g.apiURL+"/user", token, &ghUser); err != nil {
		return Profile{}, err
	}
	var emails []struct {
		Email    string `json:"email"`
		Primary  bool   `json:"primary"`
		Verified bool   `json:"verified"`
	}
	if err := g.getJSON(ctx, g.apiURL+"/user/emails", token, &emails); err != nil {
		return Profile{}, err
	}
	email := ""
	for _, candidate := range emails {
		if candidate.Primary && candidate.Verified {
			email = strings.TrimSpace(candidate.Email)
			break
		}
	}
	if email == "" {
		for _, candidate := range emails {
			if candidate.Verified {
				email = strings.TrimSpace(candidate.Email)
				break
			}
		}
	}
	if ghUser.ID <= 0 {
		return Profile{}, ErrProfile.WithDetail("user ID is invalid")
	}
	if email == "" {
		return Profile{}, ErrProfile.WithDetail("verified email is unavailable")
	}
	name := strings.TrimSpace(ghUser.Name)
	if name == "" {
		name = ghUser.Login
	}
	return Profile{
		Email:      email,
		Name:       name,
		AvatarURL:  ghUser.AvatarURL,
		ProviderID: strconv.FormatInt(ghUser.ID, 10),
	}, nil
}

func (g *GitHub) getJSON(ctx context.Context, endpoint, token string, dst any) error {
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, endpoint, nil)
	if err != nil {
		return ErrRequestFailed.WithErr(err)
	}
	req.Header.Set("Authorization", "Bearer "+token)
	req.Header.Set("Accept", "application/vnd.github+json")
	req.Header.Set("X-GitHub-Api-Version", "2022-11-28")
	resp, err := g.client.Do(req)
	if err != nil {
		return ErrRequestFailed.WithErr(err)
	}
	defer resp.Body.Close()
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		_, _ = io.Copy(io.Discard, io.LimitReader(resp.Body, 4<<10))
		return ErrRequestFailed.WithDetail(resp.Status)
	}
	if err := json.NewDecoder(io.LimitReader(resp.Body, 1<<20)).Decode(dst); err != nil {
		return ErrResponse.WithErr(err)
	}
	return nil
}
