import { Badge, Button } from "@/shared/components/ui"
import type { Member } from "@/shared/types"

export type EditableRole = Exclude<Member["role"], "owner">

type MemberListProps = {
  members: Member[]
  loading: boolean
  canManage: boolean
  pendingIDs: ReadonlySet<string>
  onRoleChange: (member: Member, role: EditableRole) => void
  onRemove: (member: Member) => void
}

export function MemberList({
  members,
  loading,
  canManage,
  pendingIDs,
  onRoleChange,
  onRemove,
}: MemberListProps) {
  return (
    <div className="member-list">
      {loading ? (
        <p className="muted member-empty">Loading members…</p>
      ) : members.length === 0 ? (
        <p className="muted member-empty">No members found.</p>
      ) : (
        members.map((member) => {
          const pending = pendingIDs.has(member.id)
          const className = ["member-row", pending && "pending"].filter(Boolean).join(" ")

          return (
            <div className={className} key={member.id}>
              <div>
                <strong>{member.name}</strong>
                <span>{member.email}</span>
              </div>
              <div className="member-actions">
                {canManage && member.role !== "owner" ? (
                  <>
                    <select
                      value={member.role}
                      disabled={pending}
                      aria-label={"Role for " + member.email}
                      onChange={(event) => onRoleChange(member, event.target.value as EditableRole)}
                    >
                      <option value="member">member</option>
                      <option value="admin">admin</option>
                    </select>
                    <Button type="button" variant="ghost" disabled={pending} onClick={() => onRemove(member)}>
                      Remove
                    </Button>
                  </>
                ) : (
                  <Badge>{member.role}</Badge>
                )}
              </div>
            </div>
          )
        })
      )}
    </div>
  )
}
