package deviceauth

import (
	"context"
	"strings"
	"time"

	"github.com/box1o/woki/pkg/api"
	"github.com/box1o/woki/pkg/id"
)

func (s *Service) Create(_ context.Context, clientName string) (api.DeviceCodeResponse, error) {
	clientName = strings.TrimSpace(clientName)
	if clientName == "" {
		clientName = "Woki CLI"
	}
	if len(clientName) > 100 {
		return api.DeviceCodeResponse{}, ErrInvalidCode.WithDetail("client name must not exceed 100 characters")
	}

	now := time.Now().UTC()

	s.mu.Lock()
	defer s.mu.Unlock()
	s.cleanupLocked(now)
	if len(s.byDevice) >= maxOutstandingDevices {
		return api.DeviceCodeResponse{}, ErrCapacityReached
	}
	deviceCode, deviceHash, err := s.uniqueDeviceTokenLocked()
	if err != nil {
		return api.DeviceCodeResponse{}, err
	}
	userCode, err := s.uniqueUserCodeLocked()
	if err != nil {
		return api.DeviceCodeResponse{}, err
	}

	expiresAt := now.Add(s.deviceTTL)
	state := &deviceState{
		DeviceHash: deviceHash,
		UserCode:   userCode,
		ClientName: clientName,
		Status:     statusPending,
		ExpiresAt:  expiresAt,
	}
	s.byDevice[deviceHash] = state
	s.byUserCode[userCode] = deviceHash

	verificationURI := s.frontendURL + "/device"
	return api.DeviceCodeResponse{
		DeviceCode:              deviceCode,
		UserCode:                userCode,
		VerificationURI:         verificationURI,
		VerificationURIComplete: verificationURI + "?code=" + userCode,
		ExpiresIn:               int(s.deviceTTL.Seconds()),
		Interval:                pollInterval,
	}, nil
}

func (s *Service) Inspect(_ context.Context, userCode string) (api.DeviceRequest, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.cleanupLocked(time.Now())
	state, err := s.stateByUserCodeLocked(userCode)
	if err != nil {
		return api.DeviceRequest{}, err
	}
	if state.Status != statusPending {
		return api.DeviceRequest{}, ErrAlreadyHandled
	}

	return api.DeviceRequest{
		UserCode:   state.UserCode,
		ClientName: state.ClientName,
		Status:     state.Status.String(),
		ExpiresAt:  state.ExpiresAt,
	}, nil
}

func (s *Service) Approve(_ context.Context, userCode string, ownerID id.ID) error {
	if !ownerID.Valid() {
		return ErrOwnerRequired
	}

	s.mu.Lock()
	defer s.mu.Unlock()

	s.cleanupLocked(time.Now())
	state, err := s.stateByUserCodeLocked(userCode)
	if err != nil {
		return err
	}
	if state.Status == statusApproved && state.OwnerID == ownerID {
		return nil
	}
	if state.Status != statusPending {
		return ErrAlreadyHandled
	}

	state.Status = statusApproved
	state.OwnerID = ownerID
	return nil
}

func (s *Service) Deny(_ context.Context, userCode string, ownerID id.ID) error {
	if !ownerID.Valid() {
		return ErrOwnerRequired
	}

	s.mu.Lock()
	defer s.mu.Unlock()

	s.cleanupLocked(time.Now())
	state, err := s.stateByUserCodeLocked(userCode)
	if err != nil {
		return err
	}
	if state.Status != statusPending {
		return ErrAlreadyHandled
	}

	state.Status = statusDenied
	state.OwnerID = ownerID
	return nil
}
