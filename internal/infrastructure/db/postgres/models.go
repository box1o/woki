package postgres

import "time"

type DBUser struct {
	ID         string    `gorm:"type:varchar(36);primaryKey"`
	Email      string    `gorm:"size:254;not null"`
	Name       string    `gorm:"size:100;not null"`
	AvatarURL  string    `gorm:"size:2048"`
	Provider   string    `gorm:"size:32;not null;uniqueIndex:idx_user_provider,priority:1"`
	ProviderID string    `gorm:"size:255;not null;uniqueIndex:idx_user_provider,priority:2"`
	CreatedAt  time.Time `gorm:"not null"`
	UpdatedAt  time.Time `gorm:"not null"`
}

func (DBUser) TableName() string { return "users" }

type DBWorkspace struct {
	ID        string    `gorm:"type:varchar(36);primaryKey"`
	Name      string    `gorm:"size:100;not null"`
	OwnerID   string    `gorm:"type:varchar(36);not null;index"`
	CreatedAt time.Time `gorm:"not null"`
	UpdatedAt time.Time `gorm:"not null"`
}

func (DBWorkspace) TableName() string { return "workspaces" }

type DBMember struct {
	ID          string    `gorm:"type:varchar(36);primaryKey"`
	UserID      string    `gorm:"type:varchar(36);not null;index;uniqueIndex:idx_member_workspace_user,priority:2"`
	WorkspaceID string    `gorm:"type:varchar(36);not null;index;uniqueIndex:idx_member_workspace_user,priority:1"`
	Email       string    `gorm:"size:254;not null"`
	Name        string    `gorm:"size:100;not null"`
	AvatarURL   string    `gorm:"size:2048"`
	Role        string    `gorm:"size:16;not null"`
	CreatedAt   time.Time `gorm:"not null"`
	UpdatedAt   time.Time `gorm:"not null"`
}

func (DBMember) TableName() string { return "workspace_members" }

type DBCredential struct {
	ID         string    `gorm:"type:varchar(36);primaryKey"`
	OwnerID    string    `gorm:"type:varchar(36);not null;index"`
	ClientName string    `gorm:"size:100;not null"`
	TokenHash  string    `gorm:"size:64;uniqueIndex;not null"`
	CreatedAt  time.Time `gorm:"not null"`
	ExpiresAt  time.Time `gorm:"not null;index"`
}

func (DBCredential) TableName() string { return "cli_credentials" }
