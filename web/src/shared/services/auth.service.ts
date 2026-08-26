import { request } from "./api"
import type { AuthStatus, DeviceRequest } from "@/shared/types"

const apiURL = (import.meta.env.VITE_WOKI_API_URL ?? "http://localhost:8080").replace(/\/$/, "")

export const authService = {
  status: () => request<AuthStatus>("/auth/status"),

  devLogin: (email: string, name: string) =>
    request<AuthStatus>("/auth/dev", {
      method: "POST",
      body: JSON.stringify({ email, name }),
    }),

  logout: () => request<void>("/auth/logout", { method: "POST" }),

  googleURL: (returnTo = "/") =>
    `${apiURL}/auth/google?return_to=${encodeURIComponent(returnTo)}`,

  githubURL: (returnTo = "/") =>
    `${apiURL}/auth/github?return_to=${encodeURIComponent(returnTo)}`,

  device: (code: string) =>
    request<DeviceRequest>(`/auth/device/request?code=${encodeURIComponent(code)}`),

  approve: (userCode: string) =>
    request<void>("/auth/device/approve", {
      method: "POST",
      body: JSON.stringify({ user_code: userCode }),
    }),

  deny: (userCode: string) =>
    request<void>("/auth/device/deny", {
      method: "POST",
      body: JSON.stringify({ user_code: userCode }),
    }),
}
