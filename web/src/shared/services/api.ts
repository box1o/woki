import type { APIError } from "@/shared/types"

const baseURL = (import.meta.env.VITE_WOKI_API_URL ?? "http://localhost:3000").replace(/\/$/, "")
const requestTimeoutMs = 20_000

export class HTTPError extends Error {
  constructor(
    readonly status: number,
    readonly code: string,
    message: string,
    readonly detail?: string,
  ) {
    super(message)
    this.name = "HTTPError"
  }
}

export async function request<T>(path: string, init: RequestInit = {}): Promise<T> {
  const headers = new Headers(init.headers)
  headers.set("Accept", "application/json")
  if (init.body && !headers.has("Content-Type")) {
    headers.set("Content-Type", "application/json")
  }

  const controller = new AbortController()
  const timeout = window.setTimeout(() => controller.abort(), requestTimeoutMs)
  const abortFromCaller = () => controller.abort()
  init.signal?.addEventListener("abort", abortFromCaller, { once: true })

  let response: Response
  try {
    response = await fetch(baseURL + path, {
      ...init,
      headers,
      credentials: "include",
      signal: controller.signal,
    })
  } catch (error) {
    const timedOut = controller.signal.aborted && !init.signal?.aborted
    throw new HTTPError(
      0,
      timedOut ? "REQUEST_TIMEOUT" : "NETWORK_ERROR",
      timedOut
        ? "The API request timed out"
        : error instanceof Error
          ? error.message
          : "Network request failed",
    )
  } finally {
    window.clearTimeout(timeout)
    init.signal?.removeEventListener("abort", abortFromCaller)
  }

  if (!response.ok) {
    let code = "HTTP_ERROR"
    let message = response.statusText || "Request failed"
    let detail: string | undefined
    try {
      const body = (await response.json()) as APIError
      if (body.error?.code) {
        code = body.error.code
      }
      if (body.error?.message) {
        message = body.error.message
      }
      detail = body.error?.detail
    } catch {
      // Keep the HTTP status fallback when the response is not a Woki error envelope.
    }
    throw new HTTPError(response.status, code, message, detail)
  }

  if (response.status === 204) {
    return undefined as T
  }

  try {
    return (await response.json()) as T
  } catch {
    throw new HTTPError(response.status, "RESPONSE_INVALID", "The API returned an invalid response")
  }
}
