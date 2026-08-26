package postgres

import (
	"context"
	"errors"
	"strings"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/pkg/id"
	"gorm.io/gorm"
)

type UserRepository struct{ db *gorm.DB }

func NewUserRepository(db *gorm.DB) *UserRepository { return &UserRepository{db: db} }

func (r *UserRepository) Create(ctx context.Context, value *user.User) error {
	if value == nil {
		return user.ErrNotFound
	}
	if err := value.Validate(); err != nil {
		return err
	}
	if err := r.db.WithContext(ctx).Create(toDBUser(value)).Error; err != nil {
		if errors.Is(err, gorm.ErrDuplicatedKey) {
			return user.ErrAlreadyExists
		}
		return user.ErrDatabaseOperation.WithErr(err)
	}
	return nil
}
func (r *UserRepository) Update(ctx context.Context, value *user.User) error {
	if value == nil {
		return user.ErrNotFound
	}
	if err := value.Validate(); err != nil {
		return err
	}
	res := r.db.WithContext(ctx).Model(&DBUser{}).Where("id = ?", value.ID.String()).Updates(toDBUser(value))
	if res.Error != nil {
		if errors.Is(res.Error, gorm.ErrDuplicatedKey) {
			return user.ErrAlreadyExists
		}
		return user.ErrDatabaseOperation.WithErr(res.Error)
	}
	if res.RowsAffected == 0 {
		return user.ErrNotFound
	}
	return nil
}
func (r *UserRepository) FindByID(ctx context.Context, value id.ID) (*user.User, error) {
	var row DBUser
	if err := r.db.WithContext(ctx).First(&row, "id = ?", value.String()).Error; err != nil {
		return nil, mapUserErr(err)
	}
	return fromDBUser(&row), nil
}
func (r *UserRepository) FindByEmail(ctx context.Context, email string) (*user.User, error) {
	var row DBUser
	if err := r.db.WithContext(ctx).Where("lower(email) = ?", strings.ToLower(strings.TrimSpace(email))).First(&row).Error; err != nil {
		return nil, mapUserErr(err)
	}
	return fromDBUser(&row), nil
}
func (r *UserRepository) FindByProvider(ctx context.Context, provider user.Provider, providerID string) (*user.User, error) {
	var row DBUser
	if err := r.db.WithContext(ctx).Where("provider = ? AND provider_id = ?", string(provider), strings.TrimSpace(providerID)).First(&row).Error; err != nil {
		return nil, mapUserErr(err)
	}
	return fromDBUser(&row), nil
}
func (r *UserRepository) Search(ctx context.Context, query string, limit int) ([]*user.User, error) {
	query = strings.ToLower(strings.TrimSpace(query))
	if query == "" || limit <= 0 {
		return []*user.User{}, nil
	}
	if limit > 100 {
		limit = 100
	}

	prefixPattern := escapeLike(query) + "%"
	var rows []DBUser
	db := r.db.WithContext(ctx)
	if err := db.
		Where(`lower(email) LIKE ? ESCAPE '\' OR lower(name) LIKE ? ESCAPE '\'`, prefixPattern, prefixPattern).
		Order(gorm.Expr("CASE WHEN lower(email) = ? THEN 0 WHEN lower(email) LIKE ? ESCAPE '\\' THEN 1 ELSE 2 END", query, prefixPattern)).
		Order("lower(name) ASC").
		Order("lower(email) ASC").
		Limit(limit).
		Find(&rows).Error; err != nil {
		return nil, user.ErrDatabaseOperation.WithErr(err)
	}

	if len(rows) < limit {
		containsPattern := "%" + escapeLike(query) + "%"
		remaining := limit - len(rows)
		ids := make([]string, 0, len(rows))
		for i := range rows {
			ids = append(ids, rows[i].ID)
		}
		var extra []DBUser
		fallback := db.Where(`lower(email) LIKE ? ESCAPE '\' OR lower(name) LIKE ? ESCAPE '\'`, containsPattern, containsPattern)
		if len(ids) > 0 {
			fallback = fallback.Where("id NOT IN ?", ids)
		}
		if err := fallback.
			Order("lower(name) ASC").
			Order("lower(email) ASC").
			Limit(remaining).
			Find(&extra).Error; err != nil {
			return nil, user.ErrDatabaseOperation.WithErr(err)
		}
		rows = append(rows, extra...)
	}

	values := make([]*user.User, 0, len(rows))
	for i := range rows {
		values = append(values, fromDBUser(&rows[i]))
	}
	return values, nil
}

func escapeLike(value string) string {
	replacer := strings.NewReplacer(`\`, `\\`, `%`, `\%`, `_`, `\_`)
	return replacer.Replace(value)
}

func mapUserErr(err error) error {
	if errors.Is(err, gorm.ErrRecordNotFound) {
		return user.ErrNotFound
	}
	return user.ErrDatabaseOperation.WithErr(err)
}
func toDBUser(v *user.User) *DBUser {
	return &DBUser{ID: v.ID.String(), Email: v.Email, Name: v.Name, AvatarURL: v.AvatarURL, Provider: string(v.Provider), ProviderID: v.ProviderID, CreatedAt: v.CreatedAt, UpdatedAt: v.UpdatedAt}
}
func fromDBUser(v *DBUser) *user.User {
	return &user.User{ID: id.ID(v.ID), Email: v.Email, Name: v.Name, AvatarURL: v.AvatarURL, Provider: user.Provider(v.Provider), ProviderID: v.ProviderID, CreatedAt: v.CreatedAt, UpdatedAt: v.UpdatedAt}
}
