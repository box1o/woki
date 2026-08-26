import type { HTMLAttributes, PropsWithChildren } from "react"

export function Badge({ className = "", children, ...props }: PropsWithChildren<HTMLAttributes<HTMLSpanElement>>) {
  return (
    <span className={`badge ${className}`.trim()} {...props}>
      {children}
    </span>
  )
}
