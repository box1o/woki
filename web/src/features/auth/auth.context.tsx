import { useCallback, useEffect, useMemo, useState, type PropsWithChildren } from "react"
import { authService, HTTPError } from "@/shared/services"
import type { User } from "@/shared/types"
import { AuthContext } from "./auth.context-value"

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
    () => ({ user, loading, error, refresh, logout }),
    [user, loading, error, refresh, logout],
  )

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}
