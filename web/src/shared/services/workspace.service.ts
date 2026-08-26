import { request } from "./api"
import type { Member, User, Workspace } from "@/shared/types"

export const workspaceService = {
  list: () => request<Workspace[]>("/workspaces"),

  create: (name: string) =>
    request<Workspace>("/workspaces", {
      method: "POST",
      body: JSON.stringify({ name }),
    }),

  remove: (workspaceID: string) =>
    request<void>(`/workspaces/${encodeURIComponent(workspaceID)}`, {
      method: "DELETE",
    }),

  members: (workspaceID: string) =>
    request<Member[]>(`/workspaces/${encodeURIComponent(workspaceID)}/members`),

  memberCandidates: (workspaceID: string, query: string, limit = 8, signal?: AbortSignal) =>
    request<User[]>(
      `/workspaces/${encodeURIComponent(workspaceID)}/member-candidates?q=${encodeURIComponent(query)}&limit=${limit}`,
      { signal },
    ),

  addMember: (workspaceID: string, email: string, role: string) =>
    request<Member>(`/workspaces/${encodeURIComponent(workspaceID)}/members`, {
      method: "POST",
      body: JSON.stringify({ email, role }),
    }),

  removeMember: (workspaceID: string, memberID: string) =>
    request<void>(
      `/workspaces/${encodeURIComponent(workspaceID)}/members/${encodeURIComponent(memberID)}`,
      { method: "DELETE" },
    ),

  updateRole: (workspaceID: string, memberID: string, role: string) =>
    request<Member>(
      `/workspaces/${encodeURIComponent(workspaceID)}/members/${encodeURIComponent(memberID)}`,
      {
        method: "PATCH",
        body: JSON.stringify({ role }),
      },
    ),
}
