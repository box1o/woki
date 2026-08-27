import { Navigate, useLocation, useSearchParams } from "react-router"
import { Alert, Button } from "@/shared/components/ui"
import { authService } from "@/shared/services"
import { useAuth } from "./auth.hook"

export function AuthPage() {
  const { user } = useAuth()
  const location = useLocation()
  const [searchParams] = useSearchParams()
  const from = safeLocalPath(
    searchParams.get("return_to") ?? (location.state as { from?: string } | null)?.from,
  )
  const oauthError = oauthErrorMessage(searchParams.get("oauth_error"))

  if (user) {
    return <Navigate to={from} replace />
  }

  return (
    <main className="center-page">
      <section className="auth-panel" aria-labelledby="sign-in-title">
        <div>
          <span className="eyebrow">WOKI</span>
          <h1 id="sign-in-title">Sign in</h1>
          <p className="muted">
            Authenticate to manage workspaces and authorize CLI sessions.
          </p>
        </div>

        <Button
          type="button"
          onClick={() => {
            window.location.assign(authService.googleURL(from))
          }}
        >
          Continue with Google
        </Button>

        {oauthError && <Alert variant="error">{oauthError}</Alert>}
      </section>
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
