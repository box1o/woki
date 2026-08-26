/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_WOKI_API_URL?: string
  readonly VITE_WOKI_DEV_AUTH?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
