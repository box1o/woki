# Coding style

Woki follows standard Go conventions with several practices inspired by the Tailscale codebase: explicit ownership, small packages, narrow interfaces, early returns, precise names, and comments that explain contracts rather than restate syntax.

## Go

- Keep domain code independent of transport and persistence.
- Accept interfaces at the consumer boundary; return concrete implementations from constructors.
- Use `pkg/errors` for domain/application semantics; enrich structured errors with stable codes, safe details, and underlying causes. Use standard `errors.Is`/`errors.As` for comparison and unwrapping.
- Never log and silently convert an error to success.
- Keep HTTP status/error-code mapping centralized at the HTTP boundary; never expose internal causes in public responses.
- Keep mutable state protected by a clearly owned lock. Do not hold service locks while performing durable I/O.
- Copy values returned by in-memory repositories so callers cannot mutate repository state without an explicit update.
- Make persistence atomic and validate data before exposing restored state.
- Use cryptographic randomness for sessions, OAuth state, device codes, and bearer tokens.
- Run `gofmt`, `go vet`, unit/integration tests, race tests, and shuffled repeated tests before release.

## TypeScript/React

- Keep product behavior inside `features`; reusable primitives and transport clients belong in `shared`.
- Use strict TypeScript and type-only imports where appropriate.
- Do not navigate or perform side effects during render.
- Keep hook dependencies explicit.
- Surface network failures separately from unauthenticated state.
- Preserve accessible labels/focus states for icon controls and form elements.
- Keep component wrappers transparent: caller `className` values must compose rather than overwrite base styles.

## Commits

Use `type(scope): short description` with `init`, `feat`, `chore`, or `docs`. One commit represents one coherent change.
