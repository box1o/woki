import { Navigate, Route, Routes } from "react-router"
import { AuthPage, AuthProtected } from "@/features/auth"
import { DevicePage } from "@/features/device-auth"
import { HomePage } from "@/features/home"
import { WorkspacePage } from "@/features/workspace"
import { Layout } from "./layout"

export function Router() {
  return (
    <Routes>
      <Route path="/auth" element={<AuthPage />} />
      <Route
        element={
          <AuthProtected>
            <Layout />
          </AuthProtected>
        }
      >
        <Route index element={<HomePage />} />
        <Route path="device" element={<DevicePage />} />
        <Route path="workspaces" element={<WorkspacePage />} />
      </Route>
      <Route path="*" element={<Navigate to="/" replace />} />
    </Routes>
  )
}
