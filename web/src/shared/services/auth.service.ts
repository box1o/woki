import { request } from "./api"
import type { AuthStatus, DeviceRequest } from "@/shared/types"

const apiURL = (import.meta.env.VITE_WOKI_API_URL ?? "http://localhost:3000").replace(/\/$/, "")

export const authService = {
  status: () => request<AuthStatus>("/auth/status"),

  logout: () => request<void>("/auth/logout", { method: "POST" }),

  googleURL: (returnTo = "/") =>
    `${apiURL}/auth/google?return_to=${encodeURIComponent(returnTo)}`,

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
