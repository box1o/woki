package memory

import (
	"context"
	"strings"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/pkg/id"
)

func (s *Store) CreateCredential(_ context.Context, value *domaincli.Credential) error {
	if value == nil {
		return domaincli.ErrCredentialNotFound
	}
	if err := value.Validate(); err != nil {
		return err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.credentials[value.ID]; ok {
		return domaincli.ErrCredentialAlreadyExists
	}
	if _, ok := s.credentialsByHash[value.TokenHash]; ok {
		return domaincli.ErrCredentialAlreadyExists
	}
	s.credentials[value.ID] = cloneCredential(value)
	s.credentialsByHash[value.TokenHash] = value.ID
	return nil
}
func (s *Store) FindCredentialByID(_ context.Context, credentialID id.ID) (*domaincli.Credential, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	c, ok := s.credentials[credentialID]
	if !ok {
		return nil, domaincli.ErrCredentialNotFound
	}
	return cloneCredential(c), nil
}
func (s *Store) FindCredentialByTokenHash(_ context.Context, hash string) (*domaincli.Credential, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	credentialID, ok := s.credentialsByHash[strings.TrimSpace(hash)]
	if !ok {
		return nil, domaincli.ErrCredentialNotFound
	}
	return cloneCredential(s.credentials[credentialID]), nil
}
func (s *Store) DeleteCredential(_ context.Context, credentialID id.ID) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	c, ok := s.credentials[credentialID]
	if !ok {
		return domaincli.ErrCredentialNotFound
	}
	delete(s.credentialsByHash, c.TokenHash)
	delete(s.credentials, credentialID)
	return nil
}
