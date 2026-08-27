import { createContext } from "react"
import type { User } from "@/shared/types"

export type AuthContextValue = {
  user: User | null
  loading: boolean
  error: string | null
  refresh: () => Promise<void>
  logout: () => Promise<void>
}

export const AuthContext = createContext<AuthContextValue | null>(null)
