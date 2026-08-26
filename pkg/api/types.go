// Package api contains transport contracts shared by the API, CLI, and web app.
package api

import "time"

const (
	ErrorCodeDeviceAuthorizationPending = "DEVICE_AUTHORIZATION_PENDING"
)

type User struct {
	ID        string `json:"id"`
	Email     string `json:"email"`
	Name      string `json:"name"`
	AvatarURL string `json:"avatar_url,omitempty"`
}

type AuthStatus struct {
	Authenticated bool  `json:"authenticated"`
	User          *User `json:"user,omitempty"`
}

type DeviceCodeResponse struct {
	DeviceCode              string `json:"device_code"`
	UserCode                string `json:"user_code"`
	VerificationURI         string `json:"verification_uri"`
	VerificationURIComplete string `json:"verification_uri_complete"`
	ExpiresIn               int    `json:"expires_in"`
	Interval                int    `json:"interval"`
}

type DeviceRequest struct {
	UserCode   string    `json:"user_code"`
	ClientName string    `json:"client_name"`
	Status     string    `json:"status"`
	ExpiresAt  time.Time `json:"expires_at"`
}

type DeviceTokenResponse struct {
	AccessToken string    `json:"access_token"`
	TokenType   string    `json:"token_type"`
	ExpiresAt   time.Time `json:"expires_at"`
	Owner       User      `json:"owner"`
}

type Workspace struct {
	ID        string    `json:"id"`
	Name      string    `json:"name"`
	OwnerID   string    `json:"owner_id"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

type Member struct {
	ID          string    `json:"id"`
	UserID      string    `json:"user_id"`
	WorkspaceID string    `json:"workspace_id"`
	Email       string    `json:"email"`
	Name        string    `json:"name"`
	AvatarURL   string    `json:"avatar_url,omitempty"`
	Role        string    `json:"role"`
	CreatedAt   time.Time `json:"created_at"`
	UpdatedAt   time.Time `json:"updated_at"`
}

type Error struct {
	Code    string `json:"code"`
	Message string `json:"message"`
	Detail  string `json:"detail,omitempty"`
}

type ErrorResponse struct {
	Error Error `json:"error"`
}
