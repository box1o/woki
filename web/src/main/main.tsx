import { StrictMode } from "react"
import { createRoot } from "react-dom/client"
import "@/shared/styles/index.css"
import { ErrorBoundary } from "./boundary"
import { Providers } from "./providers"
import { Router } from "./router"

const root = document.getElementById("root")
if (!root) {
  throw new Error("root element not found")
}

createRoot(root).render(
  <StrictMode>
    <ErrorBoundary>
      <Providers>
        <Router />
      </Providers>
    </ErrorBoundary>
  </StrictMode>,
)
