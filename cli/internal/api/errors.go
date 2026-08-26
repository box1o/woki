package api

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrURLInvalid     = apperrors.New("CLI_API_URL_INVALID", "CLI API URL is invalid")
	ErrRequestEncode  = apperrors.New("CLI_API_REQUEST_ENCODE_FAILED", "failed to encode CLI API request")
	ErrRequestBuild   = apperrors.New("CLI_API_REQUEST_BUILD_FAILED", "failed to build CLI API request")
	ErrRequestFailed  = apperrors.New("CLI_API_REQUEST_FAILED", "CLI API request failed")
	ErrResponseDecode = apperrors.New("CLI_API_RESPONSE_DECODE_FAILED", "failed to decode CLI API response")
)
