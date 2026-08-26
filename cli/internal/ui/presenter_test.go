package ui

import (
	"bytes"
	"encoding/json"
	"strings"
	"testing"
)

func TestTableJSON(t *testing.T) {
	var out bytes.Buffer
	p := New(&out, Mode{JSON: true})
	if err := p.Table(
		[]string{"ID", "Name"},
		[][]string{{"one", "Personal"}},
	); err != nil {
		t.Fatal(err)
	}
	var got []map[string]string
	if err := json.Unmarshal(out.Bytes(), &got); err != nil {
		t.Fatalf("output is not JSON: %v\n%s", err, out.String())
	}
	if len(got) != 1 || got[0]["id"] != "one" || got[0]["name"] != "Personal" {
		t.Fatalf("unexpected output: %#v", got)
	}
}

func TestTableRejectsMismatchedRows(t *testing.T) {
	p := New(&bytes.Buffer{}, Mode{})
	if err := p.Table([]string{"ID"}, [][]string{{"one", "extra"}}); err == nil {
		t.Fatal("Table unexpectedly accepted a mismatched row")
	}
}

func TestQuietSuppressesOutput(t *testing.T) {
	var out bytes.Buffer
	p := New(&out, Mode{Quiet: true})
	if err := p.Success("done", [2]string{"ID", "one"}); err != nil {
		t.Fatal(err)
	}
	if err := p.Table([]string{"ID"}, [][]string{{"one"}}); err != nil {
		t.Fatal(err)
	}
	if strings.TrimSpace(out.String()) != "" {
		t.Fatalf("quiet output=%q", out.String())
	}
}

func TestSelectReadsNumberedChoice(t *testing.T) {
	var out bytes.Buffer
	index, err := Select(strings.NewReader("2\n"), &out, "Pick workspace", []Option{
		{Label: "personal"},
		{Label: "project"},
	}, 0)
	if err != nil {
		t.Fatal(err)
	}
	if index != 1 {
		t.Fatalf("index=%d; want 1", index)
	}
	if !strings.Contains(out.String(), "* 1) personal") {
		t.Fatalf("output=%q", out.String())
	}
}
