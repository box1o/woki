import { useCallback, useEffect, useState, type FormEvent } from "react"
import { useNavigate, useSearchParams } from "react-router"
import { Alert, Button, Card, Input } from "@/shared/components/ui"
import { authService } from "@/shared/services"
import type { DeviceRequest } from "@/shared/types"

const deviceCodeLength = 8

export function DevicePage() {
  const [params] = useSearchParams()
  const navigate = useNavigate()
  const initialCode = normalizeCode(params.get("code") ?? "")
  const [code, setCode] = useState(initialCode)
  const [request, setRequest] = useState<DeviceRequest | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [busy, setBusy] = useState(false)

  const inspect = useCallback(async (value: string) => {
    const normalized = normalizeCode(value)
    if (normalized.length !== deviceCodeLength) {
      setRequest(null)
      setError("Enter the 8-character code printed by the CLI")
      return
    }

    setBusy(true)
    setError(null)
    try {
      setRequest(await authService.device(normalized))
      setCode(normalized)
    } catch (err) {
      setRequest(null)
      setError(message(err, "Unable to load device request"))
    } finally {
      setBusy(false)
    }
  }, [])

  useEffect(() => {
    if (initialCode.length !== deviceCodeLength) {
      return
    }

    const task = window.setTimeout(() => {
      void inspect(initialCode)
    }, 0)

    return () => window.clearTimeout(task)
  }, [initialCode, inspect])

  async function submitCode(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    await inspect(code)
  }

  async function decide(approve: boolean) {
    if (!request) {
      return
    }

    setBusy(true)
    setError(null)
    try {
      if (approve) {
        await authService.approve(request.user_code)
      } else {
        await authService.deny(request.user_code)
      }
      navigate("/", { replace: true })
    } catch (err) {
      setError(message(err, "Unable to update authorization"))
    } finally {
      setBusy(false)
    }
  }

  return (
    <main className="center-page">
      <Card className="device-card">
        <span className="eyebrow">CLI AUTHORIZATION</span>
        <h1>Connect Woki CLI</h1>

        {!request ? (
          <>
            <p className="muted">Enter the code printed by the CLI.</p>
            <form className="inline-form" onSubmit={submitCode}>
              <Input
                aria-label="Device code"
                value={code}
                onChange={(event) => setCode(normalizeCode(event.target.value))}
                placeholder="ABCDEFGH"
                maxLength={deviceCodeLength}
                autoComplete="off"
                spellCheck={false}
                inputMode="text"
              />
              <Button type="submit" disabled={busy || code.length !== deviceCodeLength}>
                {busy ? "Checking…" : "Continue"}
              </Button>
            </form>
          </>
        ) : (
          <>
            <p>
              Authorize <strong>{request.client_name}</strong> to access your Woki account?
            </p>
            <div className="code-box">{request.user_code}</div>
            <p className="muted small-text">
              This request expires {new Date(request.expires_at).toLocaleString()}.
            </p>
            <div className="actions">
              <Button
                type="button"
                variant="secondary"
                disabled={busy}
                onClick={() => void decide(false)}
              >
                Deny
              </Button>
              <Button type="button" disabled={busy} onClick={() => void decide(true)}>
                {busy ? "Updating…" : "Authorize"}
              </Button>
            </div>
          </>
        )}

        {error && <Alert variant="error">{error}</Alert>}
      </Card>
    </main>
  )
}

function normalizeCode(value: string) {
  return value.toUpperCase().replace(/[^A-Z2-9]/g, "").slice(0, deviceCodeLength)
}

function message(error: unknown, fallback: string) {
  return error instanceof Error ? error.message : fallback
}
