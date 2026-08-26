package memory

import (
	"context"

	domaincli "github.com/box1o/woki/internal/domain/cli"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/pkg/id"
)

type UserRepository struct{ Store *Store }

func (r UserRepository) Create(c context.Context, v *user.User) error {
	return r.Store.CreateUser(c, v)
}
func (r UserRepository) Update(c context.Context, v *user.User) error {
	return r.Store.UpdateUser(c, v)
}
func (r UserRepository) FindByID(c context.Context, v id.ID) (*user.User, error) {
	return r.Store.FindUserByID(c, v)
}
func (r UserRepository) FindByEmail(c context.Context, v string) (*user.User, error) {
	return r.Store.FindUserByEmail(c, v)
}
func (r UserRepository) FindByProvider(
	c context.Context,
	provider user.Provider,
	providerID string,
) (*user.User, error) {
	return r.Store.FindUserByProvider(c, provider, providerID)
}
func (r UserRepository) Search(c context.Context, query string, limit int) ([]*user.User, error) {
	return r.Store.SearchUsers(c, query, limit)
}

type WorkspaceRepository struct{ Store *Store }

func (r WorkspaceRepository) CreateWithOwner(c context.Context, w *workspace.Workspace, m *workspace.Member) error {
	return r.Store.CreateWorkspaceWithOwner(c, w, m)
}
func (r WorkspaceRepository) FindByOwnerAndName(c context.Context, o id.ID, n string) (*workspace.Workspace, error) {
	return r.Store.FindWorkspaceByOwnerAndName(c, o, n)
}
func (r WorkspaceRepository) ListForUser(c context.Context, u id.ID) ([]*workspace.Workspace, error) {
	return r.Store.ListWorkspacesForUser(c, u)
}
func (r WorkspaceRepository) Get(c context.Context, w id.ID) (*workspace.Workspace, error) {
	return r.Store.GetWorkspace(c, w)
}
func (r WorkspaceRepository) Delete(c context.Context, w id.ID) error {
	return r.Store.DeleteWorkspace(c, w)
}
func (r WorkspaceRepository) FindMember(c context.Context, w, u id.ID) (*workspace.Member, error) {
	return r.Store.FindWorkspaceMember(c, w, u)
}
func (r WorkspaceRepository) GetMember(c context.Context, w, m id.ID) (*workspace.Member, error) {
	return r.Store.GetWorkspaceMember(c, w, m)
}
func (r WorkspaceRepository) ListMembers(c context.Context, w id.ID) ([]*workspace.Member, error) {
	return r.Store.ListWorkspaceMembers(c, w)
}
func (r WorkspaceRepository) AddMember(c context.Context, m *workspace.Member) error {
	return r.Store.AddWorkspaceMember(c, m)
}
func (r WorkspaceRepository) RemoveMember(c context.Context, w, m id.ID) error {
	return r.Store.RemoveWorkspaceMember(c, w, m)
}
func (r WorkspaceRepository) UpdateMemberRole(c context.Context, w, m id.ID, role workspace.Role) (*workspace.Member, error) {
	return r.Store.UpdateWorkspaceMemberRole(c, w, m, role)
}

type CredentialRepository struct{ Store *Store }

func (r CredentialRepository) Create(c context.Context, v *domaincli.Credential) error {
	return r.Store.CreateCredential(c, v)
}
func (r CredentialRepository) FindByID(c context.Context, v id.ID) (*domaincli.Credential, error) {
	return r.Store.FindCredentialByID(c, v)
}
func (r CredentialRepository) FindByTokenHash(c context.Context, v string) (*domaincli.Credential, error) {
	return r.Store.FindCredentialByTokenHash(c, v)
}
func (r CredentialRepository) Delete(c context.Context, v id.ID) error {
	return r.Store.DeleteCredential(c, v)
}
