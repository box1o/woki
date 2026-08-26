import { LogOut } from "lucide-react"
import { Link } from "react-router"
import { useAuth } from "@/features/auth"
import { Button } from "@/shared/components/ui"
import { ThemeSwitcher } from "@/shared/components/theme"

export function MainHeader() {
  const { user, logout } = useAuth()

  return (
    <header className="main-header">
      <Link className="brand" to="/" aria-label="Woki home">
        <span className="brand-dot" />
        woki
      </Link>
      <div className="header-actions">
        {user && <span className="user-label">{user.email}</span>}
        <ThemeSwitcher />
        {user && (
          <Button
            type="button"
            variant="ghost"
            className="icon-button"
            onClick={() => void logout()}
            aria-label="Sign out"
          >
            <LogOut size={16} />
          </Button>
        )}
      </div>
    </header>
  )
}
