# Woki Extension SDK

WIT is the extension API contract for new work. The checked-in files in
`modules/extension/wit/` define the host imports and guest lifecycle exports.

The current raw C ABI remains supported as the compatibility lowering used by
the desktop Wasmtime backend and the Emscripten web backend. Keep the raw symbol
names stable until generated bindings replace the hand-written headers.

## Guest headers

| Header | Purpose |
|--------|---------|
| `macros.h` | `WOKI_IMPORT` / `WOKI_EXPORT` for Clang wasm builds |
| `version.h` | `WOKI_EXT_API_VERSION` |
| `woki_limits.h` | Shared size limits for guest and host |
| `types.h` | Log levels and status codes |
| `host.h` | Host import API documentation |
| `host_imports.h` | Host imports with wasm import attributes |
| `ext.h` | Guest lifecycle exports |
| `guest_alloc.h` | Optional `ext_alloc` / `ext_free` buffer pool |

Generate guest bindings with a pinned `wit-bindgen` version:

```bash
wit-bindgen c modules/extension/wit/world.wit \
  --out-dir modules/extension/sdk/generated \
  -w extension
```

The host still implements imports manually. WIT prevents the host, SDK, and docs
from drifting apart; it does not generate the C++ `HostApi` bodies.

Guest exports:

- `ext_api_version`
- `ext_init`
- `ext_on_tick`
- `ext_on_event`
- `ext_on_command` when the manifest contributes commands
- `ext_on_unload`
- `ext_alloc` / `ext_free` optional (include `guest_alloc.h`)

Host import module:

```text
woki_host
```

In-repo extensions should use `add_wokiext()` from `cmake/ExtensionWasm.cmake`.
It applies the correct wasm flags, export list, and `-fno-builtin`.

Minimal guest source:

```cpp
#include "version.h"
#include "ext.h"
#include "host_imports.h"
#include "guest_alloc.h"

extern "C" {

WOKI_EXPORT("ext_api_version")
uint32_t ext_api_version(void) { return WOKI_EXT_API_VERSION; }

// ext_init, ext_on_tick, ext_on_event, ext_on_unload ...

} // extern "C"
```

This recipe intentionally avoids C++ standard library dependencies for the
first milestone. Keep extension code C ABI friendly until the generated WIT SDK
is committed and adopted by first-party extensions.

Web support:

- Studio links `modules/extension/web/woki_ext.js` on Emscripten.
- The web backend loads `extension.wasm` from the Emscripten filesystem.
- Source extensions under repo `extensions/` are preloaded at `/extensions` in
  web builds.
- Guest imports use the same permission model as native.
