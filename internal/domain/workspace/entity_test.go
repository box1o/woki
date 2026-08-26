package workspace

import (
	"errors"
	"strings"
	"testing"

	"github.com/box1o/woki/pkg/id"
)

func TestRoleCapabilities(t *testing.T) {
	if !RoleOwner.CanManageMembers() || !RoleAdmin.CanManageMembers() || RoleMember.CanManageMembers() {
		t.Fatal("unexpected role capability")
	}
	if _, err := ParseRole("invalid"); err == nil {
		t.Fatal("ParseRole unexpectedly succeeded")
	}
}

func TestMemberRequiresValidIdentifiers(t *testing.T) {
	workspaceID := id.New()
	if _, err := NewMember("bad", workspaceID, "a@example.com", "A", RoleMember); err == nil {
		t.Fatal("invalid user ID accepted")
	}
}

func TestWorkspaceValidation(t *testing.T) {
	ownerID := id.New()
	workspace, err := New("  Engineering  ", ownerID)
	if err != nil {
		t.Fatal(err)
	}
	if workspace.Name != "Engineering" {
		t.Fatalf("name=%q", workspace.Name)
	}
	if _, err := New("   ", ownerID); !errors.Is(err, ErrNameEmpty) {
		t.Fatalf("New(empty)=%v; want ErrNameEmpty", err)
	}
	if _, err := New(strings.Repeat("x", maxWorkspaceNameLength+1), ownerID); !errors.Is(err, ErrNameTooLong) {
		t.Fatalf("New(long)=%v; want ErrNameTooLong", err)
	}
}

func TestMemberAndRoleValidation(t *testing.T) {
	userID, workspaceID := id.New(), id.New()
	member, err := NewMember(userID, workspaceID, "USER@example.com", " User ", RoleAdmin)
	if err != nil {
		t.Fatal(err)
	}
	if member.Email != "user@example.com" || member.Name != "User" || !member.Role.CanManageMembers() {
		t.Fatalf("unexpected member: %+v", member)
	}
	if role, err := ParseRole(" ADMIN "); err != nil || role != RoleAdmin {
		t.Fatalf("ParseRole()=%q,%v", role, err)
	}
	if _, err := ParseRole("root"); !errors.Is(err, ErrInvalidRole) {
		t.Fatalf("ParseRole(root)=%v; want ErrInvalidRole", err)
	}
}
