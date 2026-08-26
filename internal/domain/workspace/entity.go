package workspace

import (
	"net/mail"
	"strings"
	"time"

	"github.com/box1o/woki/pkg/id"
)

const (
	maxWorkspaceNameLength = 100
	maxMemberNameLength    = 100
	maxMemberEmailLength   = 254
)

type Role string

const (
	RoleOwner  Role = "owner"
	RoleAdmin  Role = "admin"
	RoleMember Role = "member"
)

func ParseRole(value string) (Role, error) {
	switch role := Role(strings.ToLower(strings.TrimSpace(value))); role {
	case RoleOwner, RoleAdmin, RoleMember:
		return role, nil
	default:
		return "", ErrInvalidRole
	}
}

func (r Role) CanManageMembers() bool {
	return r == RoleOwner || r == RoleAdmin
}

type Workspace struct {
	ID        id.ID     `json:"id"`
	Name      string    `json:"name"`
	OwnerID   id.ID     `json:"owner_id"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

func New(name string, ownerID id.ID) (*Workspace, error) {
	now := time.Now().UTC()
	w := &Workspace{
		ID:        id.New(),
		Name:      strings.TrimSpace(name),
		OwnerID:   ownerID,
		CreatedAt: now,
		UpdatedAt: now,
	}
	if err := w.Validate(); err != nil {
		return nil, err
	}
	return w, nil
}

func (w Workspace) Validate() error {
	if !w.ID.Valid() {
		return ErrNotFound
	}
	name := strings.TrimSpace(w.Name)
	if name == "" {
		return ErrNameEmpty
	}
	if len(name) > maxWorkspaceNameLength {
		return ErrNameTooLong
	}
	if !w.OwnerID.Valid() {
		return ErrOwnerRequired
	}
	return nil
}

type Member struct {
	ID          id.ID     `json:"id"`
	UserID      id.ID     `json:"user_id"`
	WorkspaceID id.ID     `json:"workspace_id"`
	Email       string    `json:"email"`
	Name        string    `json:"name"`
	AvatarURL   string    `json:"avatar_url,omitempty"`
	Role        Role      `json:"role"`
	CreatedAt   time.Time `json:"created_at"`
	UpdatedAt   time.Time `json:"updated_at"`
}

func NewMember(
	userID, workspaceID id.ID,
	email, name string,
	role Role,
) (*Member, error) {
	now := time.Now().UTC()
	m := &Member{
		ID:          id.New(),
		UserID:      userID,
		WorkspaceID: workspaceID,
		Email:       strings.ToLower(strings.TrimSpace(email)),
		Name:        strings.TrimSpace(name),
		Role:        role,
		CreatedAt:   now,
		UpdatedAt:   now,
	}
	if err := m.Validate(); err != nil {
		return nil, err
	}
	return m, nil
}

func (m Member) Validate() error {
	if !m.ID.Valid() {
		return ErrMemberNotFound
	}
	if !m.UserID.Valid() {
		return ErrMemberUserRequired
	}
	if !m.WorkspaceID.Valid() {
		return ErrNotFound
	}
	if err := validateMemberEmail(m.Email); err != nil {
		return err
	}
	name := strings.TrimSpace(m.Name)
	if name == "" {
		return ErrMemberNameEmpty
	}
	if len(name) > maxMemberNameLength {
		return ErrMemberNameTooLong
	}
	_, err := ParseRole(string(m.Role))
	return err
}

func validateMemberEmail(email string) error {
	email = strings.ToLower(strings.TrimSpace(email))
	if email == "" || len(email) > maxMemberEmailLength {
		return ErrMemberEmailInvalid
	}
	address, err := mail.ParseAddress(email)
	if err != nil || !strings.EqualFold(address.Address, email) {
		return ErrMemberEmailInvalid
	}
	return nil
}
