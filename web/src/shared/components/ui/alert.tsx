import type { HTMLAttributes, PropsWithChildren } from "react"

type Props = PropsWithChildren<HTMLAttributes<HTMLDivElement>> & {
  variant?: "default" | "error"
}

export function Alert({ className = "", variant = "default", children, ...props }: Props) {
  return (
    <div
      className={`alert alert-${variant} ${className}`.trim()}
      role={variant === "error" ? "alert" : undefined}
      {...props}
    >
      {children}
    </div>
  )
}
