import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  type FormEvent,
} from "react"
import { useAuth } from "@/features/auth"
import { Alert, Badge, Button, Input } from "@/shared/components/ui"
import { workspaceService } from "@/shared/services"
import type { Member, User, Workspace } from "@/shared/types"
import { WorkspaceList } from "./workspace-list"

type EditableRole = Exclude<Member["role"], "owner">

export function WorkspacePage() {
  const { user } = useAuth()
  const [workspaces, setWorkspaces] = useState<Workspace[]>([])
  const [selectedID, setSelectedID] = useState("")
  const [members, setMembers] = useState<Member[]>([])
  const [name, setName] = useState("")
  const [email, setEmail] = useState("")
  const [selectedCandidate, setSelectedCandidate] = useState<User | null>(null)
  const [candidates, setCandidates] = useState<User[]>([])
  const [candidateLoading, setCandidateLoading] = useState(false)
  const [candidateQueried, setCandidateQueried] = useState(false)
  const [candidateError, setCandidateError] = useState<string | null>(null)
  const [activeCandidate, setActiveCandidate] = useState(0)
  const [role, setRole] = useState<EditableRole>("member")
  const [loading, setLoading] = useState(true)
  const [membersLoading, setMembersLoading] = useState(false)
  const [creating, setCreating] = useState(false)
  const [deletingID, setDeletingID] = useState<string | null>(null)
  const [addingMember, setAddingMember] = useState(false)
  const [pendingMemberIDs, setPendingMemberIDs] = useState<Set<string>>(new Set())
  const [error, setError] = useState<string | null>(null)
  const memberRequest = useRef(0)
  const selectedIDRef = useRef("")

  const selected = useMemo(
    () => workspaces.find((workspace) => workspace.id === selectedID) ?? null,
    [workspaces, selectedID],
  )
  const currentMember = members.find((member) => member.user_id === user?.id)
  const canManageMembers = currentMember?.role === "owner" || currentMember?.role === "admin"

  const load = useCallback(async () => {
    setLoading(true)
    try {
      const list = await workspaceService.list()
      setWorkspaces(list)
      setSelectedID((current) =>
        current && list.some((workspace) => workspace.id === current)
          ? current
          : (list[0]?.id ?? ""),
      )
      setError(null)
    } catch (err) {
      setError(message(err))
    } finally {
      setLoading(false)
    }
  }, [])

  const loadMembers = useCallback(async (workspaceID: string) => {
    const requestID = ++memberRequest.current
    if (!workspaceID) {
      setMembers([])
      setMembersLoading(false)
      return
    }

    setMembersLoading(true)
    try {
      const result = await workspaceService.members(workspaceID)
      if (memberRequest.current !== requestID) {
        return
      }
      setMembers(result)
      setError(null)
    } catch (err) {
      if (memberRequest.current === requestID) {
        setMembers([])
        setError(message(err))
      }
    } finally {
      if (memberRequest.current === requestID) {
        setMembersLoading(false)
      }
    }
  }, [])

  useEffect(() => {
    const task = window.setTimeout(() => {
      void load()
    }, 0)

    return () => window.clearTimeout(task)
  }, [load])

  useEffect(() => {
    selectedIDRef.current = selectedID

    const task = window.setTimeout(() => {
      void loadMembers(selectedID)
    }, 0)

    return () => window.clearTimeout(task)
  }, [loadMembers, selectedID])

  useEffect(() => {
    const task = window.setTimeout(() => {
      setEmail("")
      setSelectedCandidate(null)
      setCandidates([])
      setCandidateQueried(false)
      setCandidateError(null)
      setActiveCandidate(0)
    }, 0)

    return () => window.clearTimeout(task)
  }, [selectedID])

  useEffect(() => {
    const query = email.trim()
    const controller = new AbortController()
    const timer = window.setTimeout(() => {
      if (
        !selected ||
        !canManageMembers ||
        query.length < 2 ||
        (selectedCandidate && selectedCandidate.email === query)
      ) {
        setCandidates([])
        setCandidateLoading(false)
        setCandidateQueried(false)
        setCandidateError(null)
        setActiveCandidate(0)
        return
      }

      setCandidateLoading(true)
      setCandidateQueried(false)
      setCandidateError(null)
      workspaceService
        .memberCandidates(selected.id, query, 8, controller.signal)
        .then((result) => {
          if (!controller.signal.aborted) {
            setCandidates(result)
            setActiveCandidate(0)
            setCandidateError(null)
          }
        })
        .catch((err) => {
          if (!controller.signal.aborted) {
            setCandidates([])
            setCandidateError(message(err))
          }
        })
        .finally(() => {
          if (!controller.signal.aborted) {
            setCandidateLoading(false)
            setCandidateQueried(true)
          }
        })
    }, 250)

    return () => {
      window.clearTimeout(timer)
      controller.abort()
    }
  }, [canManageMembers, email, selected, selectedCandidate])

  function invalidateMemberLoad() {
    memberRequest.current += 1
    setMembersLoading(false)
  }

  function setMemberPending(memberID: string, pending: boolean) {
    setPendingMemberIDs((current) => {
      const next = new Set(current)
      if (pending) {
        next.add(memberID)
      } else {
        next.delete(memberID)
      }
      return next
    })
  }

  async function create(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    const workspaceName = name.trim()
    if (creating || !workspaceName || !user) {
      return
    }

    const optimisticID = `optimistic:${crypto.randomUUID()}`
    const now = new Date().toISOString()
    const optimistic: Workspace = {
      id: optimisticID,
      name: workspaceName,
      owner_id: user.id,
      created_at: now,
      updated_at: now,
    }

    const selectionAtStart = selectedID
    setCreating(true)
    setError(null)
    setName("")
    setWorkspaces((current) => [...current, optimistic])

    try {
      const created = await workspaceService.create(workspaceName)
      setWorkspaces((current) =>
        current.map((workspace) => (workspace.id === optimisticID ? created : workspace)),
      )
      setSelectedID((current) => (current === selectionAtStart ? created.id : current))
    } catch (err) {
      setWorkspaces((current) => current.filter((workspace) => workspace.id !== optimisticID))
      setName((current) => current || workspaceName)
      setError(message(err))
    } finally {
      setCreating(false)
    }
  }

  async function addMember(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    if (!selected || addingMember || selected.id.startsWith("optimistic:")) {
      return
    }

    const memberEmail = email.trim().toLowerCase()
    if (!memberEmail) {
      return
    }

    const workspaceID = selected.id
    const optimisticID = `optimistic:${crypto.randomUUID()}`
    const now = new Date().toISOString()
    const optimistic: Member = {
      id: optimisticID,
      user_id: selectedCandidate?.id ?? optimisticID,
      workspace_id: workspaceID,
      email: memberEmail,
      name: selectedCandidate?.name ?? memberEmail,
      role,
      created_at: now,
      updated_at: now,
    }
    const previousCandidate = selectedCandidate
    const previousRole = role

    invalidateMemberLoad()
    setAddingMember(true)
    setMemberPending(optimisticID, true)
    setError(null)
    setMembers((current) => [...current, optimistic])
    setEmail("")
    setSelectedCandidate(null)
    setCandidates([])
    setCandidateQueried(false)
    setCandidateError(null)

    try {
      const created = await workspaceService.addMember(workspaceID, memberEmail, role)
      if (selectedIDRef.current === workspaceID) {
        setMembers((current) =>
          current.map((member) => (member.id === optimisticID ? created : member)),
        )
      }
    } catch (err) {
      if (selectedIDRef.current === workspaceID) {
        setMembers((current) => current.filter((member) => member.id !== optimisticID))
        setEmail((current) => current || memberEmail)
        setSelectedCandidate((current) => current ?? previousCandidate)
        setRole(previousRole)
        setError(message(err))
      }
    } finally {
      setMemberPending(optimisticID, false)
      setAddingMember(false)
    }
  }

  async function deleteSelected() {
    if (
      !selected ||
      deletingID === selected.id ||
      selected.id.startsWith("optimistic:") ||
      !window.confirm(`Delete ${selected.name}?`)
    ) {
      return
    }

    const deleted = selected
    const deletedIndex = workspaces.findIndex((workspace) => workspace.id === deleted.id)
    const fallbackID =
      workspaces[deletedIndex + 1]?.id ?? workspaces[deletedIndex - 1]?.id ?? ""

    invalidateMemberLoad()
    setDeletingID(deleted.id)
    setError(null)
    setWorkspaces((current) => current.filter((workspace) => workspace.id !== deleted.id))
    setSelectedID((current) => (current === deleted.id ? fallbackID : current))
    setMembers([])

    try {
      await workspaceService.remove(deleted.id)
    } catch (err) {
      setWorkspaces((current) => {
        if (current.some((workspace) => workspace.id === deleted.id)) {
          return current
        }
        const index = Math.max(0, Math.min(deletedIndex, current.length))
        return [...current.slice(0, index), deleted, ...current.slice(index)]
      })
      setSelectedID((current) => (current === fallbackID ? deleted.id : current))
      setError(message(err))
    } finally {
      setDeletingID(null)
    }
  }

  async function updateRole(member: Member, nextRole: EditableRole) {
    if (!selected || pendingMemberIDs.has(member.id) || member.id.startsWith("optimistic:")) {
      return
    }

    const workspaceID = selected.id
    const previous = member
    invalidateMemberLoad()
    setMemberPending(member.id, true)
    setError(null)
    setMembers((current) =>
      current.map((item) =>
        item.id === member.id
          ? { ...item, role: nextRole, updated_at: new Date().toISOString() }
          : item,
      ),
    )

    try {
      const updated = await workspaceService.updateRole(workspaceID, member.id, nextRole)
      if (selectedIDRef.current === workspaceID) {
        setMembers((current) =>
          current.map((item) => (item.id === member.id ? updated : item)),
        )
      }
    } catch (err) {
      if (selectedIDRef.current === workspaceID) {
        setMembers((current) =>
          current.map((item) => (item.id === member.id ? previous : item)),
        )
        setError(message(err))
      }
    } finally {
      setMemberPending(member.id, false)
    }
  }

  async function removeMember(member: Member) {
    if (
      !selected ||
      pendingMemberIDs.has(member.id) ||
      member.id.startsWith("optimistic:") ||
      !window.confirm(`Remove ${member.email} from ${selected.name}?`)
    ) {
      return
    }

    const workspaceID = selected.id
    const index = members.findIndex((item) => item.id === member.id)
    invalidateMemberLoad()
    setMemberPending(member.id, true)
    setError(null)
    setMembers((current) => current.filter((item) => item.id !== member.id))

    try {
      await workspaceService.removeMember(workspaceID, member.id)
    } catch (err) {
      if (selectedIDRef.current === workspaceID) {
        setMembers((current) => {
          if (current.some((item) => item.id === member.id)) {
            return current
          }
          const restoreAt = Math.max(0, Math.min(index, current.length))
          return [...current.slice(0, restoreAt), member, ...current.slice(restoreAt)]
        })
        setError(message(err))
      }
    } finally {
      setMemberPending(member.id, false)
    }
  }

  return (
    <main className="workspace-page">
      <WorkspaceList
        workspaces={workspaces}
        selectedID={selectedID}
        userID={user?.id}
        name={name}
        loading={loading}
        creating={creating}
        onNameChange={setName}
        onCreate={create}
        onSelect={setSelectedID}
      />

      <section className="workspace-detail" aria-live="polite">
        {selected ? (
          <div className="workspace-panel">
            <div className="section-heading">
              <div>
                <span className="eyebrow">WORKSPACE</span>
                <h2>{selected.name}</h2>
              </div>
              {selected.owner_id === user?.id && (
                <Button
                  variant="danger"
                  type="button"
                  disabled={deletingID === selected.id}
                  onClick={() => void deleteSelected()}
                >
                  Delete
                </Button>
              )}
            </div>

            <h3>Members</h3>
            <div className="member-list">
              {membersLoading ? (
                <p className="muted member-empty">Loading members…</p>
              ) : members.length === 0 ? (
                <p className="muted member-empty">No members found.</p>
              ) : (
                members.map((member) => (
                  <div className={`member-row ${pendingMemberIDs.has(member.id) ? "pending" : ""}`} key={member.id}>
                    <div>
                      <strong>{member.name}</strong>
                      <span>{member.email}</span>
                    </div>
                    <div className="member-actions">
                      {canManageMembers && member.role !== "owner" ? (
                        <>
                          <select
                            value={member.role}
                            disabled={pendingMemberIDs.has(member.id)}
                            aria-label={`Role for ${member.email}`}
                            onChange={(event) =>
                              void updateRole(member, event.target.value as EditableRole)
                            }
                          >
                            <option value="member">member</option>
                            <option value="admin">admin</option>
                          </select>
                          <Button
                            variant="ghost"
                            type="button"
                            disabled={pendingMemberIDs.has(member.id)}
                            onClick={() => void removeMember(member)}
                          >
                            Remove
                          </Button>
                        </>
                      ) : (
                        <Badge>{member.role}</Badge>
                      )}
                    </div>
                  </div>
                ))
              )}
            </div>

            {canManageMembers && (
              <form className="member-form" onSubmit={addMember}>
                <div className="member-search">
                  <Input
                    type="text"
                    required
                    role="combobox"
                    aria-label="Search user to add"
                    aria-autocomplete="list"
                    aria-controls="member-candidates"
                    aria-expanded={candidateLoading || candidateQueried}
                    aria-activedescendant={
                      candidates[activeCandidate]
                        ? `member-candidate-${candidates[activeCandidate].id}`
                        : undefined
                    }
                    autoComplete="off"
                    placeholder="Search by name or email"
                    value={email}
                    maxLength={254}
                    onChange={(event) => {
                      setEmail(event.target.value)
                      setSelectedCandidate(null)
                      setCandidateQueried(false)
                      setCandidateError(null)
                      setActiveCandidate(0)
                      setError(null)
                    }}
                    onBlur={() => {
                      setCandidates([])
                      setCandidateQueried(false)
                      setCandidateError(null)
                    }}
                    onKeyDown={(event) => {
                      if (event.key === "Escape") {
                        setCandidates([])
                        setCandidateQueried(false)
                        setCandidateError(null)
                        return
                      }
                      if (candidates.length === 0) {
                        return
                      }
                      if (event.key === "ArrowDown") {
                        event.preventDefault()
                        setActiveCandidate((current) => (current + 1) % candidates.length)
                        return
                      }
                      if (event.key === "ArrowUp") {
                        event.preventDefault()
                        setActiveCandidate(
                          (current) => (current - 1 + candidates.length) % candidates.length,
                        )
                        return
                      }
                      if (event.key === "Enter" && candidates[activeCandidate]) {
                        event.preventDefault()
                        const candidate = candidates[activeCandidate]
                        setSelectedCandidate(candidate)
                        setEmail(candidate.email)
                        setCandidates([])
                        setCandidateQueried(false)
                        setCandidateError(null)
                      }
                    }}
                  />
                  {(candidateLoading || candidateQueried) && (
                    <div
                      id="member-candidates"
                      className="member-search-results"
                      role="listbox"
                      aria-label="Matching users"
                    >
                      {candidateLoading ? (
                        <div className="member-search-status">Searching users…</div>
                      ) : candidateError ? (
                        <div className="member-search-status member-search-error">
                          {candidateError}
                        </div>
                      ) : candidates.length === 0 ? (
                        <div className="member-search-status">No users found.</div>
                      ) : (
                        candidates.map((candidate, index) => (
                          <button
                            type="button"
                            role="option"
                            id={`member-candidate-${candidate.id}`}
                            aria-selected={index === activeCandidate}
                            className={`member-search-option ${index === activeCandidate ? "active" : ""}`}
                            key={candidate.id}
                            onMouseDown={(event) => event.preventDefault()}
                            onMouseEnter={() => setActiveCandidate(index)}
                            onClick={() => {
                              setSelectedCandidate(candidate)
                              setEmail(candidate.email)
                              setCandidates([])
                              setCandidateQueried(false)
                              setCandidateError(null)
                              setError(null)
                            }}
                          >
                            <span className="member-search-avatar" aria-hidden="true">
                              {candidate.avatar_url ? (
                                <img src={candidate.avatar_url} alt="" />
                              ) : (
                                candidate.name.slice(0, 1).toUpperCase()
                              )}
                            </span>
                            <span className="member-search-copy">
                              <strong>{candidate.name}</strong>
                              <small>{candidate.email}</small>
                            </span>
                          </button>
                        ))
                      )}
                    </div>
                  )}
                </div>
                <select
                  value={role}
                  disabled={addingMember}
                  onChange={(event) => setRole(event.target.value as EditableRole)}
                  aria-label="New member role"
                >
                  <option value="member">member</option>
                  <option value="admin">admin</option>
                </select>
                <Button type="submit" disabled={addingMember || !email.trim()}>
                  Add member
                </Button>
              </form>
            )}
          </div>
        ) : (
          <p className="muted">
            {loading ? "Loading workspaces…" : "Create or select a workspace."}
          </p>
        )}

        {error && <Alert variant="error">{error}</Alert>}
      </section>
    </main>
  )
}

function message(error: unknown) {
  return error instanceof Error ? error.message : "Unexpected error"
}
