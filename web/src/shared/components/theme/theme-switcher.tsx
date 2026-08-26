import { Moon, Sun } from "lucide-react"
import { Button } from "@/shared/components/ui"
import { useTheme } from "./theme-provider"

export function ThemeSwitcher() {
  const { theme, toggle } = useTheme()
  return (
    <Button
      type="button"
      variant="ghost"
      className="icon-button"
      onClick={toggle}
      aria-label={`Switch to ${theme === "dark" ? "light" : "dark"} theme`}
    >
      {theme === "dark" ? <Sun size={16} /> : <Moon size={16} />}
    </Button>
  )
}
