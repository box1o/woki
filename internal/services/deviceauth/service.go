// Package deviceauth implements the OAuth-style device authorization flow used by the CLI.
package deviceauth

import (
	"strings"
	"sync"
	"time"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/id"
)

const (
	pollInterval          = 2
	maxOutstandingDevices = 4096
)

type stateStatus uint8

const (
	statusPending stateStatus = iota + 1
	statusApproved
	statusDenied
	statusExchanging
)

func (s stateStatus) String() string {
	switch s {
	case statusPending:
		return "pending"
	case statusApproved:
		return "approved"
	case statusDenied:
		return "denied"
	case statusExchanging:
		return "exchanging"
	default:
		return "unknown"
	}
}

type deviceState struct {
	DeviceHash string
	UserCode   string
	ClientName string
	Status     stateStatus
	OwnerID    id.ID
	ExpiresAt  time.Time
}

type Service struct {
	mu sync.Mutex

	frontendURL   string
	deviceTTL     time.Duration
	credentialTTL time.Duration
	credentials   domaincli.Repository
	users         user.Repository

	byDevice   map[string]*deviceState
	byUserCode map[string]string
}

func New(
	frontendURL string,
	deviceTTL time.Duration,
	credentialTTL time.Duration,
	credentials domaincli.Repository,
	users user.Repository,
) *Service {
	return &Service{
		frontendURL:   strings.TrimRight(frontendURL, "/"),
		deviceTTL:     deviceTTL,
		credentialTTL: credentialTTL,
		credentials:   credentials,
		users:         users,
		byDevice:      make(map[string]*deviceState),
		byUserCode:    make(map[string]string),
	}
}
