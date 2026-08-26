// Package ui renders stable human, JSON, and quiet CLI output.
package ui

import (
	"encoding/json"
	"fmt"
	"io"
	"os"
	"strings"
)

type Mode struct {
	JSON    bool
	Quiet   bool
	NoColor bool
}

type Presenter struct {
	out   io.Writer
	mode  Mode
	color bool
}

func New(out io.Writer, mode Mode) *Presenter {
	color := !mode.NoColor && os.Getenv("NO_COLOR") == ""
	if f, ok := out.(*os.File); !ok || !isTerminal(f) {
		color = false
	}
	return &Presenter{out: out, mode: mode, color: color}
}

func (p *Presenter) Writer() io.Writer { return p.out }

func (p *Presenter) AllowsInteractive() bool { return !p.mode.JSON && !p.mode.Quiet }

func (p *Presenter) Value(v any) error {
	if p.mode.Quiet {
		return nil
	}
	if p.mode.JSON {
		return p.JSON(v)
	}
	_, err := fmt.Fprintln(p.out, v)
	return err
}

func (p *Presenter) JSON(v any) error {
	if p.mode.Quiet {
		return nil
	}
	enc := json.NewEncoder(p.out)
	enc.SetIndent("", "  ")
	return enc.Encode(v)
}

func (p *Presenter) LoginInstructions(verificationURI, userCode string) error {
	if p.mode.Quiet {
		return nil
	}
	if p.mode.JSON {
		return p.JSON(map[string]string{
			"event":            "authorization_required",
			"verification_uri": verificationURI,
			"user_code":        userCode,
		})
	}
	_, err := fmt.Fprintf(p.out, "Open %s\nCode: %s\n", verificationURI, userCode)
	return err
}

func (p *Presenter) Success(title string, fields ...[2]string) error {
	if p.mode.Quiet {
		return nil
	}
	if p.mode.JSON {
		m := map[string]any{"ok": true, "message": title}
		for _, field := range fields {
			m[jsonFieldName(field[0])] = field[1]
		}
		return p.JSON(m)
	}
	if _, err := fmt.Fprintf(p.out, "%s  %s\n", p.green("◇"), title); err != nil {
		return err
	}
	for i, field := range fields {
		prefix := "│"
		if i == len(fields)-1 {
			prefix = "└"
		}
		if _, err := fmt.Fprintf(
			p.out,
			"%s  %-10s %s\n",
			p.muted(prefix),
			field[0],
			field[1],
		); err != nil {
			return err
		}
	}
	return nil
}

func (p *Presenter) Table(headers []string, rows [][]string) error {
	if p.mode.Quiet {
		return nil
	}
	if len(headers) == 0 {
		return ErrTableInvalid.WithDetail("table must have at least one column")
	}
	for i, row := range rows {
		if len(row) != len(headers) {
			return ErrTableInvalid.WithDetail(fmt.Sprintf(
				"row %d has %d columns; want %d",
				i,
				len(row),
				len(headers),
			))
		}
	}
	if p.mode.JSON {
		objects := make([]map[string]string, 0, len(rows))
		for _, row := range rows {
			object := make(map[string]string, len(headers))
			for i, value := range row {
				object[jsonFieldName(headers[i])] = value
			}
			objects = append(objects, object)
		}
		return p.JSON(objects)
	}

	widths := make([]int, len(headers))
	for i, value := range headers {
		widths[i] = len(value)
	}
	for _, row := range rows {
		for i, value := range row {
			if len(value) > widths[i] {
				widths[i] = len(value)
			}
		}
	}
	write := func(row []string) error {
		for i, value := range row {
			if i > 0 {
				if _, err := fmt.Fprint(p.out, "  "); err != nil {
					return err
				}
			}
			if _, err := fmt.Fprintf(p.out, "%-*s", widths[i], value); err != nil {
				return err
			}
		}
		_, err := fmt.Fprintln(p.out)
		return err
	}
	if err := write(headers); err != nil {
		return err
	}
	for _, row := range rows {
		if err := write(row); err != nil {
			return err
		}
	}
	return nil
}

func jsonFieldName(value string) string {
	return strings.ToLower(strings.ReplaceAll(strings.TrimSpace(value), " ", "_"))
}

func (p *Presenter) green(v string) string {
	if !p.color {
		return v
	}
	return "\x1b[32m" + v + "\x1b[0m"
}

func (p *Presenter) muted(v string) string {
	if !p.color {
		return v
	}
	return "\x1b[90m" + v + "\x1b[0m"
}

func isTerminal(f *os.File) bool {
	info, err := f.Stat()
	return err == nil && (info.Mode()&os.ModeCharDevice) != 0
}
