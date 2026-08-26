package provider

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrRequestFailed = apperrors.New("OAUTH_REQUEST_FAILED", "OAuth provider request failed")
	ErrTokenExchange = apperrors.New("OAUTH_TOKEN_EXCHANGE_FAILED", "OAuth token exchange failed")
	ErrOAuthDenied   = apperrors.New("OAUTH_DENIED", "OAuth authorization was denied")
	ErrResponse      = apperrors.New("OAUTH_RESPONSE_INVALID", "OAuth provider returned an invalid response")
	ErrProfile       = apperrors.New("OAUTH_PROFILE_INVALID", "OAuth profile is incomplete or invalid")
)
