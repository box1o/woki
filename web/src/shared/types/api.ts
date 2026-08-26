export type User = {
  id: string
  email: string
  name: string
  avatar_url?: string
}

export type AuthStatus = {
  authenticated: boolean
  user?: User
}

export type Workspace = {
  id: string
  name: string
  owner_id: string
  created_at: string
  updated_at: string
}

export type Member = {
  id: string
  user_id: string
  workspace_id: string
  email: string
  name: string
  avatar_url?: string
  role: "owner" | "admin" | "member"
  created_at: string
  updated_at: string
}

export type DeviceRequest = {
  user_code: string
  client_name: string
  status: string
  expires_at: string
}

export type APIError = {
  error: {
    code: string
    message: string
    detail?: string
  }
}
