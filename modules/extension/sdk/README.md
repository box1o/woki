# Woki Extension SDK

Milestone A uses a raw C ABI.

Guest exports:

- `ext_api_version`
- `ext_init`
- `ext_on_tick`
- `ext_on_event`
- `ext_on_unload`

Host import module:

```text
woki_host
```

Minimal clang recipe:

```bash
clang++ --target=wasm32-unknown-unknown -std=c++23 -O2 \
  -I /path/to/woki/modules/extension/sdk \
  -Wl,--no-entry \
  -Wl,--allow-undefined \
  -Wl,--export=ext_api_version \
  -Wl,--export=ext_init \
  -Wl,--export=ext_on_tick \
  -Wl,--export=ext_on_event \
  -Wl,--export=ext_on_unload \
  -o extension.wasm \
  plugin.cpp
```

Host imports must resolve from `woki_host`. For Clang, declare imports with
`import_module("woki_host")` / `import_name("host_log")` attributes in guest
code or a thin SDK wrapper source.

This recipe intentionally avoids C++ standard library dependencies for the
first milestone. Keep extension code C ABI friendly.
