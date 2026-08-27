/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_WOKI_API_URL?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
