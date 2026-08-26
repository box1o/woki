package workspace

import (
	"github.com/box1o/woki/internal/domain/events"
	"time"
)

const (
	WorkspaceCreatedEvent       events.Type = "workspace.created"
	WorkspaceMemberAddedEvent   events.Type = "workspace.member_added"
	WorkspaceMemberRemovedEvent events.Type = "workspace.member_removed"
)

type WorkspaceCreated struct {
	WorkspaceID, WorkspaceName, OwnerEmail string
	at                                     time.Time
}

func NewWorkspaceCreated(w *Workspace, ownerEmail string) WorkspaceCreated {
	return WorkspaceCreated{WorkspaceID: w.ID.String(), WorkspaceName: w.Name, OwnerEmail: ownerEmail, at: time.Now().UTC()}
}
func (e WorkspaceCreated) Type() events.Type     { return WorkspaceCreatedEvent }
func (e WorkspaceCreated) OccurredAt() time.Time { return e.at }
func (e WorkspaceCreated) Payload() any          { return e }

type MemberAdded struct {
	WorkspaceID, WorkspaceName, UserName, UserEmail string
	at                                              time.Time
}

func NewMemberAdded(w *Workspace, m *Member) MemberAdded {
	return MemberAdded{WorkspaceID: w.ID.String(), WorkspaceName: w.Name, UserName: m.Name, UserEmail: m.Email, at: time.Now().UTC()}
}
func (e MemberAdded) Type() events.Type     { return WorkspaceMemberAddedEvent }
func (e MemberAdded) OccurredAt() time.Time { return e.at }
func (e MemberAdded) Payload() any          { return e }

type MemberRemoved struct {
	WorkspaceID, WorkspaceName, UserName, UserEmail string
	at                                              time.Time
}

func NewMemberRemoved(w *Workspace, m *Member) MemberRemoved {
	return MemberRemoved{WorkspaceID: w.ID.String(), WorkspaceName: w.Name, UserName: m.Name, UserEmail: m.Email, at: time.Now().UTC()}
}
func (e MemberRemoved) Type() events.Type     { return WorkspaceMemberRemovedEvent }
func (e MemberRemoved) OccurredAt() time.Time { return e.at }
func (e MemberRemoved) Payload() any          { return e }
