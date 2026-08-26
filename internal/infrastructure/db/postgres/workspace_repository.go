package postgres

import (
	"context"
	"errors"
	"strings"
	"time"

	domain "github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/pkg/id"
	"gorm.io/gorm"
)

type WorkspaceRepository struct{ db *gorm.DB }

func NewWorkspaceRepository(db *gorm.DB) *WorkspaceRepository { return &WorkspaceRepository{db: db} }

func (r *WorkspaceRepository) CreateWithOwner(ctx context.Context, w *domain.Workspace, m *domain.Member) error {
	if w == nil || m == nil {
		return domain.ErrNotFound
	}
	if err := w.Validate(); err != nil {
		return err
	}
	if err := m.Validate(); err != nil {
		return err
	}
	if m.WorkspaceID != w.ID || m.UserID != w.OwnerID || m.Role != domain.RoleOwner {
		return domain.ErrOwnerRequired
	}
	err := r.db.WithContext(ctx).Transaction(func(tx *gorm.DB) error {
		if err := tx.Create(toDBWorkspace(w)).Error; err != nil {
			return err
		}
		return tx.Create(toDBMember(m)).Error
	})
	if err != nil {
		if errors.Is(err, gorm.ErrDuplicatedKey) {
			return domain.ErrAlreadyExists
		}
		return domain.ErrDatabaseOperation.WithErr(err)
	}
	return nil
}
func (r *WorkspaceRepository) FindByOwnerAndName(ctx context.Context, owner id.ID, name string) (*domain.Workspace, error) {
	var row DBWorkspace
	err := r.db.WithContext(ctx).Where("owner_id = ? AND lower(name) = ?", owner.String(), strings.ToLower(strings.TrimSpace(name))).First(&row).Error
	if err != nil {
		return nil, mapWorkspaceErr(err)
	}
	return fromDBWorkspace(&row), nil
}
func (r *WorkspaceRepository) ListForUser(ctx context.Context, userID id.ID) ([]*domain.Workspace, error) {
	var rows []DBWorkspace
	err := r.db.WithContext(ctx).Table("workspaces w").Select("w.*").Joins("JOIN workspace_members m ON m.workspace_id = w.id").Where("m.user_id = ?", userID.String()).Order("lower(w.name), w.id").Scan(&rows).Error
	if err != nil {
		return nil, domain.ErrDatabaseOperation.WithErr(err)
	}
	out := make([]*domain.Workspace, 0, len(rows))
	for i := range rows {
		out = append(out, fromDBWorkspace(&rows[i]))
	}
	return out, nil
}
func (r *WorkspaceRepository) Get(ctx context.Context, value id.ID) (*domain.Workspace, error) {
	var row DBWorkspace
	if err := r.db.WithContext(ctx).First(&row, "id = ?", value.String()).Error; err != nil {
		return nil, mapWorkspaceErr(err)
	}
	return fromDBWorkspace(&row), nil
}
func (r *WorkspaceRepository) Delete(ctx context.Context, value id.ID) error {
	return r.db.WithContext(ctx).Transaction(func(tx *gorm.DB) error {
		if err := tx.Delete(&DBMember{}, "workspace_id = ?", value.String()).Error; err != nil {
			return domain.ErrDatabaseOperation.WithErr(err)
		}
		res := tx.Delete(&DBWorkspace{}, "id = ?", value.String())
		if res.Error != nil {
			return domain.ErrDatabaseOperation.WithErr(res.Error)
		}
		if res.RowsAffected == 0 {
			return domain.ErrNotFound
		}
		return nil
	})
}
func (r *WorkspaceRepository) FindMember(ctx context.Context, wid, uid id.ID) (*domain.Member, error) {
	var row DBMember
	if err := r.db.WithContext(ctx).Where("workspace_id = ? AND user_id = ?", wid.String(), uid.String()).First(&row).Error; err != nil {
		return nil, mapMemberErr(err)
	}
	return fromDBMember(&row), nil
}
func (r *WorkspaceRepository) GetMember(ctx context.Context, wid, mid id.ID) (*domain.Member, error) {
	var row DBMember
	err := memberQuery(r.db.WithContext(ctx)).
		Where("m.workspace_id = ? AND m.id = ?", wid.String(), mid.String()).
		Take(&row).Error
	if err != nil {
		return nil, mapMemberErr(err)
	}
	return fromDBMember(&row), nil
}
func (r *WorkspaceRepository) ListMembers(ctx context.Context, wid id.ID) ([]*domain.Member, error) {
	var count int64
	if err := r.db.WithContext(ctx).Model(&DBWorkspace{}).Where("id = ?", wid.String()).Count(&count).Error; err != nil {
		return nil, domain.ErrDatabaseOperation.WithErr(err)
	}
	if count == 0 {
		return nil, domain.ErrNotFound
	}
	var rows []DBMember
	if err := memberQuery(r.db.WithContext(ctx)).
		Where("m.workspace_id = ?", wid.String()).
		Order("CASE m.role WHEN 'owner' THEN 0 WHEN 'admin' THEN 1 ELSE 2 END").
		Order("lower(COALESCE(u.name, m.name)), lower(COALESCE(u.email, m.email))").
		Find(&rows).Error; err != nil {
		return nil, domain.ErrDatabaseOperation.WithErr(err)
	}
	out := make([]*domain.Member, 0, len(rows))
	for i := range rows {
		out = append(out, fromDBMember(&rows[i]))
	}
	return out, nil
}
func (r *WorkspaceRepository) AddMember(ctx context.Context, m *domain.Member) error {
	if m == nil {
		return domain.ErrMemberNotFound
	}
	if err := m.Validate(); err != nil {
		return err
	}
	if err := r.db.WithContext(ctx).Create(toDBMember(m)).Error; err != nil {
		if errors.Is(err, gorm.ErrDuplicatedKey) {
			return domain.ErrMemberAlreadyExists
		}
		return domain.ErrDatabaseOperation.WithErr(err)
	}
	return nil
}
func (r *WorkspaceRepository) RemoveMember(ctx context.Context, wid, mid id.ID) error {
	var row DBMember
	if err := r.db.WithContext(ctx).Where("workspace_id = ? AND id = ?", wid.String(), mid.String()).First(&row).Error; err != nil {
		return mapMemberErr(err)
	}
	if row.Role == string(domain.RoleOwner) {
		return domain.ErrOwnerRemoval
	}
	res := r.db.WithContext(ctx).Delete(&DBMember{}, "id = ?", mid.String())
	if res.Error != nil {
		return domain.ErrDatabaseOperation.WithErr(res.Error)
	}
	return nil
}
func (r *WorkspaceRepository) UpdateMemberRole(ctx context.Context, wid, mid id.ID, role domain.Role) (*domain.Member, error) {
	if role == domain.RoleOwner {
		return nil, domain.ErrOwnerRemoval
	}
	if _, err := domain.ParseRole(string(role)); err != nil {
		return nil, err
	}
	var row DBMember
	if err := r.db.WithContext(ctx).Where("workspace_id = ? AND id = ?", wid.String(), mid.String()).First(&row).Error; err != nil {
		return nil, mapMemberErr(err)
	}
	if row.Role == string(domain.RoleOwner) {
		return nil, domain.ErrOwnerRemoval
	}
	row.Role = string(role)
	row.UpdatedAt = time.Now().UTC()
	if err := r.db.WithContext(ctx).Save(&row).Error; err != nil {
		return nil, domain.ErrDatabaseOperation.WithErr(err)
	}
	return fromDBMember(&row), nil
}
func mapWorkspaceErr(err error) error {
	if errors.Is(err, gorm.ErrRecordNotFound) {
		return domain.ErrNotFound
	}
	return domain.ErrDatabaseOperation.WithErr(err)
}
func mapMemberErr(err error) error {
	if errors.Is(err, gorm.ErrRecordNotFound) {
		return domain.ErrMemberNotFound
	}
	return domain.ErrDatabaseOperation.WithErr(err)
}
func toDBWorkspace(v *domain.Workspace) *DBWorkspace {
	return &DBWorkspace{ID: v.ID.String(), Name: v.Name, OwnerID: v.OwnerID.String(), CreatedAt: v.CreatedAt, UpdatedAt: v.UpdatedAt}
}
func fromDBWorkspace(v *DBWorkspace) *domain.Workspace {
	return &domain.Workspace{ID: id.ID(v.ID), Name: v.Name, OwnerID: id.ID(v.OwnerID), CreatedAt: v.CreatedAt, UpdatedAt: v.UpdatedAt}
}
func toDBMember(v *domain.Member) *DBMember {
	return &DBMember{ID: v.ID.String(), UserID: v.UserID.String(), WorkspaceID: v.WorkspaceID.String(), Email: v.Email, Name: v.Name, AvatarURL: v.AvatarURL, Role: string(v.Role), CreatedAt: v.CreatedAt, UpdatedAt: v.UpdatedAt}
}
func fromDBMember(v *DBMember) *domain.Member {
	return &domain.Member{ID: id.ID(v.ID), UserID: id.ID(v.UserID), WorkspaceID: id.ID(v.WorkspaceID), Email: v.Email, Name: v.Name, AvatarURL: v.AvatarURL, Role: domain.Role(v.Role), CreatedAt: v.CreatedAt, UpdatedAt: v.UpdatedAt}
}

func memberQuery(db *gorm.DB) *gorm.DB {
	return db.Table("workspace_members m").
		Select(`m.id, m.user_id, m.workspace_id,
			COALESCE(u.email, m.email) AS email,
			COALESCE(u.name, m.name) AS name,
			COALESCE(NULLIF(u.avatar_url, ''), m.avatar_url) AS avatar_url,
			m.role, m.created_at, m.updated_at`).
		Joins("LEFT JOIN users u ON u.id = m.user_id")
}
