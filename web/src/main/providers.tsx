import type { PropsWithChildren } from "react"
import { BrowserRouter } from "react-router"
import { AuthProvider } from "@/features/auth"
import { ThemeProvider } from "@/shared/components/theme"

export function Providers({ children }: PropsWithChildren) {
  return (
    <ThemeProvider>
      <BrowserRouter>
        <AuthProvider>{children}</AuthProvider>
      </BrowserRouter>
    </ThemeProvider>
  )
}
