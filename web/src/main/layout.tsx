import { Outlet } from "react-router"
import { MainHeader, MainSidebar } from "@/shared/components"

export function Layout() {
  return (
    <div className="app-shell">
      <MainHeader />
      <div className="app-body">
        <aside className="sidebar">
          <MainSidebar />
        </aside>
        <div className="content">
          <Outlet />
        </div>
      </div>
    </div>
  )
}
