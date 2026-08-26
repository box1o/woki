import type { ButtonHTMLAttributes } from "react"

type Props = ButtonHTMLAttributes<HTMLButtonElement> & {
  variant?: "primary" | "secondary" | "danger" | "ghost"
}

export function Button({ className = "", variant = "primary", ...props }: Props) {
  return (
    <button
      className={`button button-${variant} ${className}`.trim()}
      {...props}
    />
  )
}
