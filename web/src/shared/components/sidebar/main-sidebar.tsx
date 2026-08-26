import { Home, Users } from "lucide-react"
import { NavLink } from "react-router"

const links = [
  { to: "/", label: "Home", icon: Home },
  { to: "/workspaces", label: "Workspaces", icon: Users },
]

export function MainSidebar() {
  return (
    <nav className="main-sidebar" aria-label="Primary navigation">
      {links.map(({ to, label, icon: Icon }) => (
        <NavLink
          key={to}
          to={to}
          end={to === "/"}
          className={({ isActive }) => `sidebar-link ${isActive ? "active" : ""}`}
          aria-label={label}
          title={label}
        >
          <Icon size={17} />
        </NavLink>
      ))}
    </nav>
  )
}
