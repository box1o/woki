package postgres

import (
	"context"
	"errors"
	"strings"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/pkg/id"
	"gorm.io/gorm"
)

type CredentialRepository struct{ db *gorm.DB }

func NewCredentialRepository(db *gorm.DB) *CredentialRepository { return &CredentialRepository{db: db} }
func (r *CredentialRepository) Create(ctx context.Context, v *domaincli.Credential) error {
	if v == nil {
		return domaincli.ErrCredentialNotFound
	}
	if err := v.Validate(); err != nil {
		return err
	}
	if err := r.db.WithContext(ctx).Create(toDBCredential(v)).Error; err != nil {
		if errors.Is(err, gorm.ErrDuplicatedKey) {
			return domaincli.ErrCredentialAlreadyExists
		}
		return domaincli.ErrDatabaseOperation.WithErr(err)
	}
	return nil
}
func (r *CredentialRepository) FindByID(ctx context.Context, value id.ID) (*domaincli.Credential, error) {
	var row DBCredential
	if err := r.db.WithContext(ctx).First(&row, "id = ?", value.String()).Error; err != nil {
		return nil, mapCredentialErr(err)
	}
	return fromDBCredential(&row), nil
}
func (r *CredentialRepository) FindByTokenHash(ctx context.Context, value string) (*domaincli.Credential, error) {
	var row DBCredential
	if err := r.db.WithContext(ctx).Where("token_hash = ?", strings.TrimSpace(value)).First(&row).Error; err != nil {
		return nil, mapCredentialErr(err)
	}
	return fromDBCredential(&row), nil
}
func (r *CredentialRepository) Delete(ctx context.Context, value id.ID) error {
	res := r.db.WithContext(ctx).Delete(&DBCredential{}, "id = ?", value.String())
	if res.Error != nil {
		return domaincli.ErrDatabaseOperation.WithErr(res.Error)
	}
	if res.RowsAffected == 0 {
		return domaincli.ErrCredentialNotFound
	}
	return nil
}
func mapCredentialErr(err error) error {
	if errors.Is(err, gorm.ErrRecordNotFound) {
		return domaincli.ErrCredentialNotFound
	}
	return domaincli.ErrDatabaseOperation.WithErr(err)
}
func toDBCredential(v *domaincli.Credential) *DBCredential {
	return &DBCredential{ID: v.ID.String(), OwnerID: v.OwnerID.String(), ClientName: v.ClientName, TokenHash: v.TokenHash, CreatedAt: v.CreatedAt, ExpiresAt: v.ExpiresAt}
}
func fromDBCredential(v *DBCredential) *domaincli.Credential {
	return &domaincli.Credential{ID: id.ID(v.ID), OwnerID: id.ID(v.OwnerID), ClientName: v.ClientName, TokenHash: v.TokenHash, CreatedAt: v.CreatedAt, ExpiresAt: v.ExpiresAt}
}
