import {
  Component,
  type ErrorInfo,
  type PropsWithChildren,
  type ReactNode,
} from "react"
import { Button, Card } from "@/shared/components/ui"

type State = {
  error: Error | null
}

export class ErrorBoundary extends Component<PropsWithChildren, State> {
  state: State = { error: null }

  static getDerivedStateFromError(error: Error): State {
    return { error }
  }

  componentDidCatch(error: Error, info: ErrorInfo) {
    console.error("Woki UI error", error, info)
  }

  render(): ReactNode {
    if (this.state.error) {
      return (
        <main className="center-page">
          <Card className="narrow stack">
            <div>
              <h1>Something went wrong</h1>
              <p className="muted">Reload the application and try again.</p>
            </div>
            <Button type="button" onClick={() => window.location.reload()}>
              Reload
            </Button>
          </Card>
        </main>
      )
    }
    return this.props.children
  }
}
