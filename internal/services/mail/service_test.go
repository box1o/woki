package mail

import (
	"context"
	"strings"
	"testing"

	domainevents "github.com/box1o/woki/internal/domain/events"
	domainmail "github.com/box1o/woki/internal/domain/mail"
	"github.com/box1o/woki/pkg/config"
)

type recordingSender struct {
	messages []domainmail.Message
}

func (s *recordingSender) Send(_ context.Context, message domainmail.Message) error {
	s.messages = append(s.messages, message)
	return nil
}

type noopBus struct{}

func (noopBus) Publish(context.Context, domainevents.Event) error       { return nil }
func (noopBus) Subscribe(domainevents.Type, domainevents.Handler) error { return nil }

func TestSendDeliversValidatedApplicationEmail(t *testing.T) {
	sender := &recordingSender{}
	svc := New(config.MailConfig{Enabled: true, Name: "Woki"}, "https://woki.example", sender, noopBus{})

	if err := svc.Send(context.Background(), "user@example.com", "Hello", "Welcome to Woki"); err != nil {
		t.Fatal(err)
	}
	if len(sender.messages) != 1 {
		t.Fatalf("messages=%d; want 1", len(sender.messages))
	}
	message := sender.messages[0]
	if len(message.To) != 1 || message.To[0] != "user@example.com" {
		t.Fatalf("unexpected recipients: %#v", message.To)
	}
	if message.Subject != "Hello" || !strings.Contains(message.Text, "Welcome to Woki") || message.HTML == "" {
		t.Fatalf("unexpected message: %#v", message)
	}
	if !strings.Contains(message.HTML, "support@woki.sh") {
		t.Fatal("HTML does not contain the default Woki support address")
	}
}

func TestSendRejectsInvalidRecipient(t *testing.T) {
	sender := &recordingSender{}
	svc := New(config.MailConfig{Enabled: true, Name: "Woki"}, "https://woki.example", sender, noopBus{})

	if err := svc.Send(context.Background(), "not-an-email", "Hello", "Body"); err == nil {
		t.Fatal("Send() succeeded for invalid recipient")
	}
	if len(sender.messages) != 0 {
		t.Fatalf("messages=%d; want 0", len(sender.messages))
	}
}

func TestWorkspaceCreatedEmailUsesBrandedTemplateAndSupportAddress(t *testing.T) {
	sender := &recordingSender{}
	svc := New(config.MailConfig{
		Enabled:   true,
		Name:      "Woki",
		SupportTo: "support@woki.sh",
	}, "https://woki.sh", sender, noopBus{})

	if err := svc.SendWorkspaceCreated(context.Background(), "owner@example.com", "platform"); err != nil {
		t.Fatal(err)
	}
	if len(sender.messages) != 1 {
		t.Fatalf("messages=%d; want 1", len(sender.messages))
	}
	message := sender.messages[0]
	for _, want := range []string{"Your workspace is ready", "platform", "support@woki.sh", "https://woki.sh/workspaces"} {
		if !strings.Contains(message.HTML, want) {
			t.Fatalf("HTML does not contain %q", want)
		}
		if !strings.Contains(message.Text, want) && want != "https://woki.sh/workspaces" {
			t.Fatalf("text does not contain %q", want)
		}
	}
	if !strings.Contains(message.Text, "https://woki.sh/workspaces") {
		t.Fatal("text fallback does not contain workspace URL")
	}
}
