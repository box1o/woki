# Woki

Woki is a workspace platform with a Go API and CLI plus a React web application.

It keeps domain logic independent from HTTP, PostgreSQL, Redis, SMTP, and OAuth. The composition root lives in `internal/application`; adapters and transport remain at the edge.

```text
cmd/api          API executable
cli              CLI executable and commands
internal/domain  entities, invariants, events, repository contracts
internal/services use cases
internal/infrastructure PostgreSQL, Redis, mail, OAuth, sessions
internal/interfaces HTTP handlers and middleware
web              React application
```

For local development, copy `.env.example`, start PostgreSQL and Redis with `docker compose up postgres redis`, then run `go run ./cmd/api` and `npm --prefix web run dev`.

Run checks with:

```bash
go vet ./...
go test ./...
go build ./cmd/api ./cli/cmd/woki
npm --prefix web run lint
npm --prefix web run build
```
