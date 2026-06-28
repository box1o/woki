# Wasmtime C API

Official docs: https://bytecodealliance.github.io/wasmtime/c-api/

## `WASMTIME_PROVIDER` (Makefile)

| Value | Behavior |
|-------|----------|
| `prebuilt` | **Default.** Download official `*-c-api` release into `3rdparty/.deps/` |
| `source` | Fetch wasmtime git + `cargo build -p wasmtime-c-api` (requires Rust) |
| `auto` | Try prebuilt first, then source |

```bash
make build                              # download prebuilt Wasmtime
make build WASMTIME_PROVIDER=source     # build from Rust (no prebuilt download)
make build WASMTIME_PROVIDER=auto       # prebuilt, fallback to Rust build
make build WOKI_EXTENSION_WITH_WASMTIME=OFF   # skip Wasmtime entirely
```

CMake cache variable: `WOKI_WASMTIME_PROVIDER` (same values).

Other options: `WOKI_WASMTIME_VERSION`, `WOKI_WASMTIME_USE_SHARED`, `WOKI_WASMTIME_ROOT`.
