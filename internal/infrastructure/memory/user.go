package memory

import (
	"context"
	"sort"
	"strings"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/id"
)

func (s *Store) CreateUser(_ context.Context, value *user.User) error {
	if value == nil {
		return user.ErrNotFound
	}
	if err := value.Validate(); err != nil {
		return err
	}

	s.mu.Lock()
	defer s.mu.Unlock()

	emailKey := normalizeEmail(value.Email)
	providerKey := providerIdentityKey(value.Provider, value.ProviderID)
	if _, exists := s.users[value.ID]; exists {
		return user.ErrAlreadyExists
	}
	if _, exists := s.usersByEmail[emailKey]; exists {
		return user.ErrAlreadyExists
	}
	if _, exists := s.usersByProvider[providerKey]; exists {
		return user.ErrAlreadyExists
	}

	s.users[value.ID] = cloneUser(value)
	s.usersByEmail[emailKey] = value.ID
	s.usersByProvider[providerKey] = value.ID
	return nil
}

func (s *Store) UpdateUser(_ context.Context, value *user.User) error {
	if value == nil {
		return user.ErrNotFound
	}
	if err := value.Validate(); err != nil {
		return err
	}

	s.mu.Lock()
	defer s.mu.Unlock()

	old, ok := s.users[value.ID]
	if !ok {
		return user.ErrNotFound
	}

	newEmailKey := normalizeEmail(value.Email)
	newProviderKey := providerIdentityKey(value.Provider, value.ProviderID)
	if existingID, exists := s.usersByEmail[newEmailKey]; exists && existingID != value.ID {
		return user.ErrAlreadyExists
	}
	if existingID, exists := s.usersByProvider[newProviderKey]; exists && existingID != value.ID {
		return user.ErrAlreadyExists
	}

	oldEmailKey := normalizeEmail(old.Email)
	if oldEmailKey != newEmailKey {
		delete(s.usersByEmail, oldEmailKey)
		s.usersByEmail[newEmailKey] = value.ID
	}
	oldProviderKey := providerIdentityKey(old.Provider, old.ProviderID)
	if oldProviderKey != newProviderKey {
		delete(s.usersByProvider, oldProviderKey)
		s.usersByProvider[newProviderKey] = value.ID
	}

	s.users[value.ID] = cloneUser(value)
	return nil
}

func (s *Store) FindUserByID(_ context.Context, value id.ID) (*user.User, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	found, ok := s.users[value]
	if !ok {
		return nil, user.ErrNotFound
	}
	return cloneUser(found), nil
}

func (s *Store) FindUserByEmail(_ context.Context, email string) (*user.User, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	userID, ok := s.usersByEmail[strings.ToLower(strings.TrimSpace(email))]
	if !ok {
		return nil, user.ErrNotFound
	}
	return cloneUser(s.users[userID]), nil
}

func (s *Store) FindUserByProvider(
	_ context.Context,
	provider user.Provider,
	providerID string,
) (*user.User, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	userID, ok := s.usersByProvider[providerIdentityKey(provider, providerID)]
	if !ok {
		return nil, user.ErrNotFound
	}
	return cloneUser(s.users[userID]), nil
}

func (s *Store) SearchUsers(ctx context.Context, query string, limit int) ([]*user.User, error) {
	if err := ctx.Err(); err != nil {
		return nil, err
	}
	query = strings.ToLower(strings.TrimSpace(query))
	if query == "" || limit <= 0 {
		return []*user.User{}, nil
	}

	s.mu.RLock()
	defer s.mu.RUnlock()

	results := make([]*user.User, 0, min(limit, len(s.users)))
	for _, value := range s.users {
		if strings.Contains(strings.ToLower(value.Email), query) ||
			strings.Contains(strings.ToLower(value.Name), query) {
			results = append(results, cloneUser(value))
		}
	}
	sort.Slice(results, func(i, j int) bool {
		iEmail := strings.ToLower(results[i].Email)
		jEmail := strings.ToLower(results[j].Email)
		iPrefix := strings.HasPrefix(iEmail, query)
		jPrefix := strings.HasPrefix(jEmail, query)
		if iPrefix != jPrefix {
			return iPrefix
		}
		if results[i].Name != results[j].Name {
			return strings.ToLower(results[i].Name) < strings.ToLower(results[j].Name)
		}
		return iEmail < jEmail
	})
	if len(results) > limit {
		results = results[:limit]
	}
	return results, nil
}
