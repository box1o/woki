import { request } from "./api"

export const mailService = {
  sendIssue: (subject: string, body: string) =>
    request<void>("/mail/issue", {
      method: "POST",
      body: JSON.stringify({ subject, body }),
    }),
}
