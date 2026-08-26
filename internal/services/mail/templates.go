package mail

import (
	"bytes"
	"html/template"
	"strings"
)

const layout = `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta name="color-scheme" content="light">
  <title>{{.Title}}</title>
</head>
<body style="margin:0;padding:0;background:#f3f4f6;color:#18181b;font-family:Inter,-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;">
  {{if .Preheader}}<div style="display:none;max-height:0;overflow:hidden;opacity:0;color:transparent;">{{.Preheader}}</div>{{end}}
  <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0" style="width:100%;background:#f3f4f6;">
    <tr>
      <td align="center" style="padding:40px 16px;">
        <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0" style="width:100%;max-width:640px;">
          <tr>
            <td style="padding:0 4px 14px;">
              <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0">
                <tr>
                  <td style="font-size:18px;font-weight:800;letter-spacing:-0.02em;color:#18181b;">woki</td>
                  <td align="right" style="font-size:12px;color:#71717a;">{{.Eyebrow}}</td>
                </tr>
              </table>
            </td>
          </tr>
          <tr>
            <td style="background:#ffffff;border:1px solid #e4e4e7;border-radius:20px;overflow:hidden;box-shadow:0 12px 36px rgba(24,24,27,0.06);">
              <div style="height:4px;background:#86efac;"></div>
              <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0">
                <tr>
                  <td style="padding:38px 40px 18px;">
                    <div style="display:inline-block;margin-bottom:16px;padding:6px 10px;border-radius:999px;background:#f0fdf4;color:#15803d;font-size:11px;font-weight:800;letter-spacing:.08em;text-transform:uppercase;">Woki</div>
                    <h1 style="margin:0 0 14px;font-size:28px;line-height:1.2;letter-spacing:-0.035em;color:#18181b;font-weight:750;">{{.Title}}</h1>
                    <p style="margin:0;font-size:15px;line-height:1.75;color:#52525b;">{{.Intro}}</p>
                  </td>
                </tr>
                {{if .DetailValue}}
                <tr>
                  <td style="padding:4px 40px 8px;">
                    <table role="presentation" width="100%" cellspacing="0" cellpadding="0" border="0" style="background:#fafafa;border:1px solid #e4e4e7;border-radius:14px;">
                      <tr>
                        <td style="padding:16px 18px;">
                          <div style="font-size:11px;font-weight:700;letter-spacing:.08em;text-transform:uppercase;color:#a1a1aa;margin-bottom:5px;">{{.DetailLabel}}</div>
                          <div style="font-size:15px;font-weight:650;color:#27272a;word-break:break-word;">{{.DetailValue}}</div>
                        </td>
                      </tr>
                    </table>
                  </td>
                </tr>
                {{end}}
                {{if .Body}}
                <tr>
                  <td style="padding:12px 40px 4px;">
                    <div style="background:#fafafa;border:1px solid #e4e4e7;border-radius:14px;padding:16px 18px;font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,monospace;font-size:13px;line-height:1.65;color:#3f3f46;white-space:pre-wrap;word-break:break-word;">{{.Body}}</div>
                  </td>
                </tr>
                {{end}}
                {{if .ActionURL}}
                <tr>
                  <td style="padding:22px 40px 8px;">
                    <table role="presentation" cellspacing="0" cellpadding="0" border="0">
                      <tr>
                        <td bgcolor="#18181b" style="border-radius:999px;">
                          <a href="{{.ActionURL}}" style="display:inline-block;padding:12px 20px;color:#ffffff;text-decoration:none;font-size:14px;font-weight:700;line-height:20px;">{{.ActionText}}</a>
                        </td>
                      </tr>
                    </table>
                  </td>
                </tr>
                {{end}}
                {{if .Outro}}
                <tr>
                  <td style="padding:22px 40px 38px;">
                    <p style="margin:0;font-size:14px;line-height:1.7;color:#71717a;">{{.Outro}}</p>
                  </td>
                </tr>
                {{else}}
                <tr><td style="height:24px;line-height:24px;font-size:1px;">&nbsp;</td></tr>
                {{end}}
              </table>
            </td>
          </tr>
          <tr>
            <td style="padding:20px 12px 0;text-align:center;font-size:12px;line-height:1.7;color:#a1a1aa;">
              <div>Sent by {{.Product}}</div>
              {{if .SupportEmail}}<div>Need help? <a href="mailto:{{.SupportEmail}}" style="color:#71717a;text-decoration:underline;">{{.SupportEmail}}</a></div>{{end}}
              <div style="margin-top:4px;">This is an automated transactional email.</div>
            </td>
          </tr>
        </table>
      </td>
    </tr>
  </table>
</body>
</html>`

type templateData struct {
	Title        string
	Preheader    string
	Eyebrow      string
	Intro        string
	ActionURL    string
	ActionText   string
	DetailLabel  string
	DetailValue  string
	Body         string
	Outro        string
	Product      string
	SupportEmail string
}

func render(data templateData) (string, error) {
	t, err := template.New("mail").Parse(layout)
	if err != nil {
		return "", ErrTemplate.WithErr(err)
	}

	var b bytes.Buffer
	if err := t.Execute(&b, data); err != nil {
		return "", ErrTemplate.WithErr(err)
	}
	return b.String(), nil
}

func renderText(data templateData) string {
	var b strings.Builder
	b.WriteString(data.Title)
	b.WriteString("\n\n")
	b.WriteString(data.Intro)
	b.WriteString("\n")

	if data.DetailValue != "" {
		b.WriteString("\n")
		if data.DetailLabel != "" {
			b.WriteString(data.DetailLabel)
			b.WriteString(": ")
		}
		b.WriteString(data.DetailValue)
		b.WriteString("\n")
	}
	if data.Body != "" {
		b.WriteString("\n")
		b.WriteString(data.Body)
		b.WriteString("\n")
	}
	if data.ActionURL != "" {
		b.WriteString("\n")
		if data.ActionText != "" {
			b.WriteString(data.ActionText)
			b.WriteString(": ")
		}
		b.WriteString(data.ActionURL)
		b.WriteString("\n")
	}
	if data.Outro != "" {
		b.WriteString("\n")
		b.WriteString(data.Outro)
		b.WriteString("\n")
	}
	if data.SupportEmail != "" {
		b.WriteString("\nNeed help? ")
		b.WriteString(data.SupportEmail)
		b.WriteString("\n")
	}

	return strings.TrimSpace(b.String())
}
