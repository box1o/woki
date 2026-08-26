import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
  type PropsWithChildren,
} from "react"
import { authService, HTTPError } from "@/shared/services"
import type { User } from "@/shared/types"

type AuthContextValue = {
  user: User | null
  loading: boolean
  error: string | null
  refresh: () => Promise<void>
  devLogin: (email: string, name: string) => Promise<void>
  logout: () => Promise<void>
}

const AuthContext = createContext<AuthContextValue | null>(null)

export function AuthProvider({ children }: PropsWithChildren) {
  const [user, setUser] = useState<User | null>(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  const refresh = useCallback(async () => {
    try {
      const status = await authService.status()
      setUser(status.user ?? null)
      setError(null)
    } catch (err) {
      if (err instanceof HTTPError && err.status === 401) {
        setUser(null)
        setError(null)
      } else {
        setError(err instanceof Error ? err.message : "Unable to reach the API")
      }
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    const task = window.setTimeout(() => {
      void refresh()
    }, 0)

    return () => window.clearTimeout(task)
  }, [refresh])

  const devLogin = useCallback(async (email: string, name: string) => {
    const status = await authService.devLogin(email, name)
    setUser(status.user ?? null)
    setError(null)
  }, [])

  const logout = useCallback(async () => {
    try {
      await authService.logout()
      setError(null)
    } catch (err) {
      setError(err instanceof Error ? err.message : "Unable to sign out cleanly")
    } finally {
      setUser(null)
    }
  }, [])

  const value = useMemo(
    () => ({ user, loading, error, refresh, devLogin, logout }),
    [user, loading, error, refresh, devLogin, logout],
  )

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

export function useAuth() {
  const value = useContext(AuthContext)
  if (!value) {
    throw new Error("useAuth must be used inside AuthProvider")
  }
  return value
}
