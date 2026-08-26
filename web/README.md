# Woki Web

React/Vite frontend for browser authentication, CLI device approval, and workspace management.

The structure intentionally stays close to Fuse:

- `src/main` owns the application shell, providers, router, and error boundary.
- `src/features` owns product behavior (`auth`, `device-auth`, `home`, `workspace`).
- `src/shared` owns reusable UI primitives, API services, types, theme support, and styles.

From the repository root:

```bash
make web-install
make dev-web
```

Verification:

```bash
make web-lint
make web-build
```
