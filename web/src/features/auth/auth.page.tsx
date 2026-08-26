import { useState, type FormEvent } from "react"
import { Navigate, useLocation, useNavigate, useSearchParams } from "react-router"
import { Alert, Button, Card, Input } from "@/shared/components/ui"
import { authService } from "@/shared/services"
import { useAuth } from "./auth.context"

export function AuthPage() {
  const { user, devLogin } = useAuth()
  const navigate = useNavigate()
  const location = useLocation()
  const [searchParams] = useSearchParams()
  const [email, setEmail] = useState("dev@example.com")
  const [name, setName] = useState("Developer")
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const devEnabled = import.meta.env.VITE_WOKI_DEV_AUTH === "true"
  const githubEnabled = import.meta.env.VITE_WOKI_GITHUB_AUTH === "true"
  const from = safeLocalPath(
    searchParams.get("return_to") ?? (location.state as { from?: string } | null)?.from,
  )
  const oauthError = oauthErrorMessage(searchParams.get("oauth_error"))

  if (user) {
    return <Navigate to={from} replace />
  }

  async function submit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    if (busy) {
      return
    }

    setBusy(true)
    setError(null)
    try {
      await devLogin(email, name)
      navigate(from, { replace: true })
    } catch (err) {
      setError(err instanceof Error ? err.message : "Sign in failed")
    } finally {
      setBusy(false)
    }
  }

  return (
    <main className="center-page">
      <Card className="auth-card">
        <div>
          <span className="eyebrow">WOKI</span>
          <h1>Sign in</h1>
          <p className="muted">
            Authenticate to manage workspaces and authorize CLI sessions.
          </p>
        </div>

        <Button
          type="button"
          disabled={busy}
          onClick={() => {
            window.location.assign(authService.googleURL(from))
          }}
        >
          Continue with Google
        </Button>

        {githubEnabled && (
          <Button
            type="button"
            variant="secondary"
            disabled={busy}
            onClick={() => {
              window.location.assign(authService.githubURL(from))
            }}
          >
            Continue with GitHub
          </Button>
        )}

        {oauthError && <Alert variant="error">{oauthError}</Alert>}

        {devEnabled && (
          <>
            <div className="divider">
              <span>development</span>
            </div>
            <form className="stack" onSubmit={submit}>
              <label>
                Email
                <Input
                  type="email"
                  required
                  autoComplete="email"
                  value={email}
                  onChange={(event) => setEmail(event.target.value)}
                />
              </label>
              <label>
                Name
                <Input
                  required
                  autoComplete="name"
                  value={name}
                  onChange={(event) => setName(event.target.value)}
                />
              </label>
              {error && <Alert variant="error">{error}</Alert>}
              <Button type="submit" disabled={busy}>
                {busy ? "Signing in…" : "Development sign in"}
              </Button>
            </form>
          </>
        )}
      </Card>
    </main>
  )
}

function safeLocalPath(value?: string) {
  if (!value || !value.startsWith("/") || value.startsWith("//") || value.includes("\\")) {
    return "/"
  }
  try {
    const resolved = new URL(value, window.location.origin)
    if (resolved.origin !== window.location.origin || resolved.hash) {
      return "/"
    }
    return resolved.pathname + resolved.search
  } catch {
    return "/"
  }
}

function oauthErrorMessage(code: string | null) {
  switch (code) {
    case "denied":
      return "Sign in was cancelled before Woki received access."
    case "identity_conflict":
      return "This email is already registered with a different sign-in method."
    case "state":
      return "The sign-in request expired or could not be verified. Please try again."
    case "provider":
      return "The identity provider could not complete sign in. Please try again."
    case "invalid_response":
      return "The identity provider returned an incomplete sign-in response."
    case "unavailable":
      return "Woki could not complete sign in right now. Please try again shortly."
    default:
      return null
  }
}
