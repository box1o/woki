import type { PropsWithChildren } from "react"
import { Navigate, useLocation } from "react-router"
import { useAuth } from "./auth.context"

export function AuthProtected({ children }: PropsWithChildren) {
  const { user, loading, error } = useAuth()
  const location = useLocation()

  if (loading) {
    return (
      <main className="center-page">
        <p className="muted">Loading…</p>
      </main>
    )
  }

  if (error) {
    return (
      <main className="center-page">
        <div className="card narrow">
          <h1>API unavailable</h1>
          <p className="muted">{error}</p>
        </div>
      </main>
    )
  }

  if (!user) {
    return (
      <Navigate
        to="/auth"
        replace
        state={{ from: location.pathname + location.search }}
      />
    )
  }

  return children
}
