# Architecture

Woki deliberately keeps the recognizable organization of Fuse while improving the boundaries between domain behavior, application services, infrastructure, HTTP transport, the CLI, and the browser UI. It is one monorepo with three first-class applications: API, web, and CLI.

## Backend dependency direction

```text
cmd/api
  -> internal/application
       -> internal/interfaces/server
       -> internal/services
       -> internal/infrastructure
            -> internal/domain
       -> pkg
```

The important rule is that domain packages do not know which database, cache, HTTP framework, OAuth provider, or UI is being used.

### `internal/domain`

Owns:

- entities and invariants
- domain events
- repository contracts
- stable domain errors

Current bounded areas are user, workspace, CLI credential, events, and mail delivery contracts.

### `internal/services`

Owns use cases and authorization decisions:

- browser account/session authentication
- CLI device authorization and bearer credentials
- workspace and membership management
- event publication
- email notification orchestration

Services depend on domain contracts rather than concrete PostgreSQL/Redis implementations.

### `internal/infrastructure`

Contains replaceable adapters:

```text
db/postgres/       GORM/PostgreSQL repositories and migrations
file/              deterministic single-process repository fallback
redis/             namespaced Redis client
session/           in-memory and Redis browser sessions
ratelimit/         in-memory and Redis-backed GCRA limiters
provider/          Google and GitHub OAuth providers
events/            process-local domain event bus
mail/              SMTP sender
```

PostgreSQL is durable business storage. Redis is intentionally used only for ephemeral/shared coordination state: sessions, device authorization, distributed locks, and rate-limit counters.

### `internal/interfaces/server`

HTTP is an adapter around the services. This layer owns:

- request decoding and response encoding
- cookies and bearer-token extraction
- CORS/security headers
- API/auth/device/mail rate limiting
- transport authentication middleware
- health/readiness endpoints
- stable domain-error to HTTP-error mapping

Transport handlers do not contain database logic.

### `internal/application`

The application package is the explicit composition root and follows the same visible staged setup style as Fuse:

```text
setupInfrastructure
setupRepositories
setupServices
setupServer
```

There is no reflection-based dependency container or service locator. Concrete infrastructure choices are visible in one place.

## Persistence

### PostgreSQL

The default adapter stores:

- users and OAuth provider identities
- workspaces
- workspace members/roles
- CLI credential hashes

Workspace creation and deletion use transactions. Unique constraints protect provider identities, emails, memberships, token hashes, and workspace names. Workspace names are additionally constrained case-insensitively per owner.

### File adapter

The file adapter is intentionally retained for tests and lightweight single-process use. It writes deterministic snapshots with atomic replacement and protected file permissions. It is not the recommended multi-instance production backend.

## Redis

Redis makes authentication/abuse-prevention state shared between API replicas.

```text
session:*                 browser sessions
device:state:*            pending CLI authorizations
device:user:*             human code -> device state
...:lock:*                device decision/exchange locks
rate:*                    GCRA theoretical-arrival-time state
```

Distributed device locks are released with compare-and-delete semantics so one worker cannot delete a lock acquired by another worker after expiry.

When Redis is explicitly disabled, the same contracts are fulfilled by in-process fallbacks.

## Authentication

### Browser

Google OAuth is the primary browser flow. The provider adapter:

1. redirects to Google with an OAuth state value,
2. exchanges the authorization code,
3. reads OpenID user information,
4. requires a verified email,
5. identifies the account by the stable Google `sub` value,
6. creates/updates the Woki user,
7. creates the default personal workspace when needed,
8. creates a Redis-backed browser session.

GitHub OAuth is an optional second provider. Development authentication is available only when explicitly enabled and is rejected by production validation.

### CLI

The CLI uses a device authorization flow:

1. CLI requests a device/user code.
2. Device state is stored in Redis with TTL.
3. User signs in through the browser and approves the code.
4. CLI polls the token endpoint.
5. At successful exchange, Woki creates the durable credential in PostgreSQL.
6. Only a SHA-256 token hash is stored server-side.
7. The CLI stores the bearer token in Secret Service or a protected local file.

Creating the durable credential at exchange time avoids leaving unreachable credentials merely because a browser approved a flow that the CLI never completed.

## Domain events and mail

Core use cases publish domain events rather than invoking SMTP directly:

```text
account.created
workspace.created
workspace.member_added
workspace.member_removed
```

The mail service subscribes to these events and renders Woki email templates. `internal/infrastructure/mail` owns SMTP transport. Gmail is configured through standard SMTP with a Google App Password.

The SMTP contract itself lives in `internal/domain/mail`, so the mail service depends on an abstraction rather than importing its infrastructure adapter. The service also exposes a validated generic `Send` method for future application use cases while the public HTTP surface remains intentionally narrow.

This keeps mail failure and presentation details out of user/workspace domain code.

## Rate limiting

`pkg/ratelimit.Limiter` is the stable contract. The Redis adapter uses `go-redis/redis_rate`'s atomic GCRA implementation and is shared across replicas. A memory adapter implements the same sustained-rate and burst semantics for development/tests. GCRA avoids fixed-window boundary bursts while keeping constant-size Redis state per identity.

Separate policies exist for:

- global API requests
- authentication flows
- device-code creation
- device-token polling
- mail submission

Rate-limit middleware uses a server-observed client IP rather than trusting arbitrary forwarded headers by default.

## Errors, config, and logging

### Errors

`pkg/errors` keeps the Fuse-style structured error model. Domains define stable codes; services wrap unknown infrastructure causes; the HTTP boundary maps errors centrally. Internal causes remain available to logs but are not serialized to clients.

### Config

`pkg/config` is strongly typed and explicitly passed through the composition root. Unlike a global singleton, it is deterministic to test and its ownership is visible. Production validation enforces important safety requirements such as PostgreSQL/Redis availability, secure cookies, and disabled development authentication.

### Logging

`pkg/log` keeps the familiar Fuse API (`Trace`, `Debug`, `Info`, `Warn`, `Error`, levels, output modes) with synchronized configuration and protected log files.

## CLI boundaries

```text
cli/cmd/woki
  -> cli/internal/commands
       -> auth/workspace use cases
       -> typed API client
       -> credential store
       -> presenter/platform adapters
```

The CLI is normal compiled Go code. It does not generate commands from configuration files. Human, JSON, quiet, color, and interactive behaviors remain presentation concerns.

## Web boundaries

```text
web/src/main       application shell, router, providers
web/src/features   auth, device authorization, home, workspace behavior
web/src/shared     UI primitives, API services, types, styles
```

The visual language stays close to Fuse: compact navigation, rounded surfaces, neutral light/dark palettes, and the green brand accent. Google sign-in is the primary production authentication action; GitHub and direct development login remain optional.

Workspace member management uses a server-authorized candidate search. The repository searches by name/email, the workspace service filters existing members, and the browser presents a debounced accessible combobox. This keeps directory access behind workspace-management authorization instead of exposing a global unauthenticated user directory.
