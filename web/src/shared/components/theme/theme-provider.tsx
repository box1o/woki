import {
  useEffect,
  useMemo,
  useState,
  type PropsWithChildren,
} from "react"
import { ThemeContext, type Theme } from "./theme.context"
const storageKey = "woki-theme"

export function ThemeProvider({ children }: PropsWithChildren) {
  const [theme, setTheme] = useState<Theme>(initialTheme)

  useEffect(() => {
    document.documentElement.classList.toggle("dark", theme === "dark")
    try {
      localStorage.setItem(storageKey, theme)
    } catch {
      // Theme persistence is optional when browser storage is unavailable.
    }
  }, [theme])

  const value = useMemo(
    () => ({
      theme,
      toggle: () => setTheme((current) => (current === "dark" ? "light" : "dark")),
    }),
    [theme],
  )

  return <ThemeContext.Provider value={value}>{children}</ThemeContext.Provider>
}

function initialTheme(): Theme {
  try {
    const stored = localStorage.getItem(storageKey)
    if (stored === "light" || stored === "dark") {
      return stored
    }
  } catch {
    // Fall back to the system preference below.
  }

  return window.matchMedia?.("(prefers-color-scheme: light)").matches ? "light" : "dark"
}
