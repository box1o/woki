import type { FormEvent } from "react"
import { Button, Input } from "@/shared/components/ui"
import type { Workspace } from "@/shared/types"

type WorkspaceListProps = {
  workspaces: Workspace[]
  selectedID: string
  userID?: string
  name: string
  loading: boolean
  creating: boolean
  onNameChange: (name: string) => void
  onCreate: (event: FormEvent<HTMLFormElement>) => void
  onSelect: (id: string) => void
}

export function WorkspaceList({
  workspaces,
  selectedID,
  userID,
  name,
  loading,
  creating,
  onNameChange,
  onCreate,
  onSelect,
}: WorkspaceListProps) {
  return (
    <section className="workspace-list" aria-label="Workspaces">
      <div className="section-heading">
        <div>
          <span className="eyebrow">WORKSPACES</span>
          <h1>Your spaces</h1>
        </div>
      </div>

      <form className="inline-form" onSubmit={onCreate}>
        <Input
          aria-label="Workspace name"
          placeholder="Workspace name"
          value={name}
          maxLength={100}
          onChange={(event) => onNameChange(event.target.value)}
          required
        />
        <Button type="submit" disabled={creating || !name.trim()}>
          Create
        </Button>
      </form>

      <div className="workspace-items">
        {loading ? (
          <p className="muted small-text">Loading workspaces…</p>
        ) : workspaces.length === 0 ? (
          <p className="muted small-text">No workspaces yet.</p>
        ) : (
          workspaces.map((workspace) => {
            const pending = workspace.id.startsWith("optimistic:")
            const className = ["workspace-item", workspace.id === selectedID && "active", pending && "pending"]
              .filter(Boolean)
              .join(" ")

            return (
              <button
                type="button"
                key={workspace.id}
                className={className}
                disabled={pending}
                onClick={() => onSelect(workspace.id)}
                aria-pressed={workspace.id === selectedID}
              >
                <span>{workspace.name}</span>
                <small>{pending ? "saving…" : workspace.owner_id === userID ? "owner" : "member"}</small>
              </button>
            )
          })
        )}
      </div>
    </section>
  )
}
