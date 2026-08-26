import {
  createContext,
  useContext,
  useEffect,
  useMemo,
  useState,
  type PropsWithChildren,
} from "react"

type Theme = "light" | "dark"
type ThemeContextValue = {
  theme: Theme
  toggle: () => void
}

const ThemeContext = createContext<ThemeContextValue | null>(null)
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

export function useTheme() {
  const value = useContext(ThemeContext)
  if (!value) {
    throw new Error("useTheme must be used inside ThemeProvider")
  }
  return value
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
