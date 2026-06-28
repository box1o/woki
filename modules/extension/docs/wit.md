# WIT extension API

WIT is the source of truth for the Woki extension API.

Files:

- `wit/host.wit` defines host services available to extensions.
- `wit/guest.wit` defines lifecycle callbacks implemented by extensions.
- `wit/world.wit` composes the extension world.

The raw `ext_*` / `host_*` ABI is the current lowering for native and web
runtime backends. Do not add new host functions only to `sdk/host.h`; add them
to WIT first, then lower them into the compatibility ABI until generated
bindings are adopted.

Permission mapping:

| Manifest permission | WIT interface |
|---------------------|---------------|
| `log` | `logging` |
| `paths` | `paths` |
| `storage` | `storage` |
| `config` | `config` |
| `events` | `events` |

Runtime backend:

Studio uses one runtime backend, `woki::ext::wasm::Backend`, created through
`wasm::Backend::Create()`. The backend owns an `Engine` implementation:

- native: `wasm::WasmtimeEngine`
- web: `wasm::WebEngine`

`WebEngine` uses the browser `WebAssembly` API through `web/woki_ext.js`. It
instantiates `extension.wasm` from the mounted filesystem, wires `woki_host`
imports, calls lifecycle exports, and forwards host storage to the Emscripten
filesystem under the extension data root.
