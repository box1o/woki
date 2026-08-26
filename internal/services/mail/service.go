package mail

import (
	"context"
	"fmt"
	stdmail "net/mail"
	"strings"
	"sync"
	"time"

	domainevents "github.com/box1o/woki/internal/domain/events"
	domainmail "github.com/box1o/woki/internal/domain/mail"
	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/pkg/config"
	"github.com/box1o/woki/pkg/log"
)

const defaultSupportEmail = "support@woki.sh"

type queuedMail struct {
	kind string
	send func(context.Context) error
}

type Service struct {
	enabled     bool
	frontendURL string
	product     string
	supportTo   string
	sender      domainmail.Sender
	bus         domainevents.Bus
	sendTimeout time.Duration
	workers     int
	jobs        chan queuedMail

	queueMu sync.RWMutex
	closed  bool
	wg      sync.WaitGroup
	ctx     context.Context
	cancel  context.CancelFunc
}

func New(cfg config.MailConfig, frontendURL string, sender domainmail.Sender, bus domainevents.Bus) *Service {
	product := strings.TrimSpace(cfg.Name)
	if product == "" {
		product = "Woki"
	}
	supportTo := strings.ToLower(strings.TrimSpace(cfg.SupportTo))
	if supportTo == "" {
		supportTo = defaultSupportEmail
	}
	queueSize := cfg.QueueSize
	if queueSize < 1 {
		queueSize = 128
	}
	workers := cfg.Workers
	if workers < 1 {
		workers = 2
	}
	sendTimeout := cfg.SendTimeout
	if sendTimeout <= 0 {
		sendTimeout = 15 * time.Second
	}
	ctx, cancel := context.WithCancel(context.Background())
	return &Service{
		enabled:     cfg.Enabled,
		frontendURL: strings.TrimRight(frontendURL, "/"),
		product:     product,
		supportTo:   supportTo,
		sender:      sender,
		bus:         bus,
		sendTimeout: sendTimeout,
		workers:     workers,
		jobs:        make(chan queuedMail, queueSize),
		ctx:         ctx,
		cancel:      cancel,
	}
}

func (s *Service) Setup() error {
	if !s.enabled {
		return nil
	}
	if s.sender == nil {
		return ErrConfiguration.WithDetail("mail sender is required")
	}
	if s.bus == nil {
		return ErrConfiguration.WithDetail("event bus is required")
	}

	subs := []struct {
		t domainevents.Type
		h domainevents.Handler
	}{
		{user.AccountCreatedEvent, s.onAccountCreated},
		{workspace.WorkspaceCreatedEvent, s.onWorkspaceCreated},
		{workspace.WorkspaceMemberAddedEvent, s.onMemberAdded},
		{workspace.WorkspaceMemberRemovedEvent, s.onMemberRemoved},
	}
	for _, sub := range subs {
		if err := s.bus.Subscribe(sub.t, sub.h); err != nil {
			return ErrConfiguration.WithErr(err)
		}
	}
	for range s.workers {
		s.wg.Add(1)
		go s.worker()
	}
	return nil
}

func (s *Service) onAccountCreated(_ context.Context, event domainevents.Event) error {
	payload, ok := event.Payload().(user.AccountCreated)
	if !ok {
		return ErrPayload
	}
	return s.enqueue("account-created", func(ctx context.Context) error {
		return s.SendAccountCreated(ctx, payload.UserEmail, payload.UserName)
	})
}

func (s *Service) onWorkspaceCreated(_ context.Context, event domainevents.Event) error {
	payload, ok := event.Payload().(workspace.WorkspaceCreated)
	if !ok {
		return ErrPayload
	}
	return s.enqueue("workspace-created", func(ctx context.Context) error {
		return s.SendWorkspaceCreated(ctx, payload.OwnerEmail, payload.WorkspaceName)
	})
}

func (s *Service) onMemberAdded(_ context.Context, event domainevents.Event) error {
	payload, ok := event.Payload().(workspace.MemberAdded)
	if !ok {
		return ErrPayload
	}
	return s.enqueue("member-added", func(ctx context.Context) error {
		return s.SendMemberAdded(ctx, payload.UserEmail, payload.UserName, payload.WorkspaceName)
	})
}

func (s *Service) onMemberRemoved(_ context.Context, event domainevents.Event) error {
	payload, ok := event.Payload().(workspace.MemberRemoved)
	if !ok {
		return ErrPayload
	}
	return s.enqueue("member-removed", func(ctx context.Context) error {
		return s.SendMemberRemoved(ctx, payload.UserEmail, payload.UserName, payload.WorkspaceName)
	})
}

func (s *Service) enqueue(kind string, send func(context.Context) error) error {
	if !s.enabled {
		return nil
	}
	if send == nil {
		return ErrPayload.WithDetail("mail job is empty")
	}
	s.queueMu.RLock()
	defer s.queueMu.RUnlock()
	if s.closed {
		return ErrShuttingDown
	}
	select {
	case s.jobs <- queuedMail{kind: kind, send: send}:
		return nil
	default:
		return ErrQueueFull.WithDetail(kind)
	}
}

func (s *Service) worker() {
	defer s.wg.Done()
	for job := range s.jobs {
		ctx, cancel := context.WithTimeout(s.ctx, s.sendTimeout)
		err := job.send(ctx)
		cancel()
		if err != nil {
			log.Error("send %s email: %v", job.kind, err)
		} else {
			log.Debug("sent %s email", job.kind)
		}
	}
}

func (s *Service) Shutdown(ctx context.Context) error {
	if !s.enabled {
		return nil
	}
	s.queueMu.Lock()
	if !s.closed {
		s.closed = true
		close(s.jobs)
	}
	s.queueMu.Unlock()

	done := make(chan struct{})
	go func() {
		s.wg.Wait()
		close(done)
	}()
	select {
	case <-done:
		s.cancel()
		return nil
	case <-ctx.Done():
		s.cancel()
		return ctx.Err()
	}
}

func (s *Service) sendTemplate(ctx context.Context, to, subject string, data templateData) error {
	if !s.enabled {
		return ErrDisabled
	}
	to = strings.ToLower(strings.TrimSpace(to))
	if err := validateRecipient(to); err != nil {
		return err
	}
	data.Product = s.product
	data.SupportEmail = s.supportTo
	html, err := render(data)
	if err != nil {
		return err
	}
	if err := s.sender.Send(ctx, domainmail.Message{
		To:      []string{to},
		Subject: subject,
		Text:    renderText(data),
		HTML:    html,
	}); err != nil {
		return ErrSend.WithErr(err)
	}
	return nil
}

func (s *Service) SendAccountCreated(ctx context.Context, to, name string) error {
	return s.sendTemplate(ctx, to, "Welcome to Woki", templateData{
		Title:      "Your Woki account is ready",
		Preheader:  "Welcome to Woki — your account is ready to use.",
		Eyebrow:    "Account created",
		Intro:      fmt.Sprintf("Hi %s, welcome to Woki. Your account was created successfully and you can start organizing workspaces right away.", name),
		ActionURL:  s.frontendURL,
		ActionText: "Open Woki",
		Outro:      "You can create workspaces, collaborate with your team, and authorize the Woki CLI from your account.",
	})
}

func (s *Service) SendWorkspaceCreated(ctx context.Context, to, name string) error {
	return s.sendTemplate(ctx, to, "Workspace created · Woki", templateData{
		Title:       "Your workspace is ready",
		Preheader:   fmt.Sprintf("The %s workspace was created successfully.", name),
		Eyebrow:     "Workspace created",
		Intro:       "Your new workspace has been created successfully and is ready for collaboration.",
		DetailLabel: "Workspace",
		DetailValue: name,
		ActionURL:   s.frontendURL + "/workspaces",
		ActionText:  "Open workspace",
		Outro:       "Invite teammates, assign roles, and manage access from the workspace members page.",
	})
}

func (s *Service) SendMemberAdded(ctx context.Context, to, userName, workspaceName string) error {
	return s.sendTemplate(ctx, to, "You joined a Woki workspace", templateData{
		Title:       "You have access to a new workspace",
		Preheader:   fmt.Sprintf("You were added to %s on Woki.", workspaceName),
		Eyebrow:     "Workspace access",
		Intro:       fmt.Sprintf("Hi %s, you were added to a Woki workspace and can start collaborating with the team.", userName),
		DetailLabel: "Workspace",
		DetailValue: workspaceName,
		ActionURL:   s.frontendURL + "/workspaces",
		ActionText:  "Open workspace",
		Outro:       "Sign in using this email address to see the workspace and your current access level.",
	})
}

func (s *Service) SendMemberRemoved(ctx context.Context, to, userName, workspaceName string) error {
	return s.sendTemplate(ctx, to, "Workspace access removed · Woki", templateData{
		Title:       "Your workspace access changed",
		Preheader:   fmt.Sprintf("Your access to %s was removed.", workspaceName),
		Eyebrow:     "Access updated",
		Intro:       fmt.Sprintf("Hi %s, your access to the workspace below has been removed.", userName),
		DetailLabel: "Workspace",
		DetailValue: workspaceName,
		Outro:       "If you believe this change was unexpected, contact the workspace owner or reach out to Woki support.",
	})
}

// Send delivers a normal application email through the configured Woki mail
// transport. Domain-specific notifications should keep using the dedicated
// methods above so their wording stays consistent, while this method provides
// one validated entry point for future application features.
func (s *Service) Send(ctx context.Context, to, subject, body string) error {
	if !s.enabled {
		return ErrDisabled
	}
	to = strings.ToLower(strings.TrimSpace(to))
	subject = strings.TrimSpace(subject)
	body = strings.TrimSpace(body)
	if err := validateRecipient(to); err != nil {
		return err
	}
	if subject == "" {
		return ErrPayload.WithDetail("subject is required")
	}
	if len(subject) > 200 {
		return ErrPayload.WithDetail("subject must not exceed 200 characters")
	}
	if body == "" {
		return ErrPayload.WithDetail("body is required")
	}
	if len(body) > 20000 {
		return ErrPayload.WithDetail("body must not exceed 20000 characters")
	}
	data := templateData{
		Title:        subject,
		Preheader:    subject,
		Eyebrow:      "Woki notification",
		Intro:        body,
		Product:      s.product,
		SupportEmail: s.supportTo,
	}
	html, err := render(data)
	if err != nil {
		return err
	}
	if err := s.sender.Send(ctx, domainmail.Message{
		To:      []string{to},
		Subject: subject,
		Text:    renderText(data),
		HTML:    html,
	}); err != nil {
		return ErrSend.WithErr(err)
	}
	return nil
}

func (s *Service) SendIssue(ctx context.Context, fromEmail, subject, body string) error {
	if !s.enabled {
		return ErrDisabled
	}
	fromEmail = strings.ToLower(strings.TrimSpace(fromEmail))
	if err := validateRecipient(fromEmail); err != nil {
		return err
	}
	subject = strings.TrimSpace(subject)
	body = strings.TrimSpace(body)
	if subject == "" || body == "" {
		return ErrPayload.WithDetail("subject and body are required")
	}
	if len(subject) > 200 {
		return ErrPayload.WithDetail("subject must not exceed 200 characters")
	}
	if len(body) > 10000 {
		return ErrPayload.WithDetail("body must not exceed 10000 characters")
	}
	data := templateData{
		Title:        "New Woki issue report",
		Preheader:    "A user submitted a new issue report.",
		Eyebrow:      "Support report",
		Intro:        "A Woki user submitted an issue that may need attention.",
		DetailLabel:  "Reported by",
		DetailValue:  fromEmail,
		Body:         body,
		Outro:        "Review the report and follow up with the sender when needed.",
		Product:      s.product,
		SupportEmail: s.supportTo,
	}
	html, err := render(data)
	if err != nil {
		return err
	}
	if err := s.sender.Send(ctx, domainmail.Message{
		To:      []string{s.supportTo},
		ReplyTo: fromEmail,
		Subject: "[WOKI ISSUE] " + subject,
		Text:    renderText(data),
		HTML:    html,
	}); err != nil {
		return ErrSend.WithErr(err)
	}
	log.Info("issue email accepted for delivery from %s", fromEmail)
	return nil
}

func validateRecipient(value string) error {
	if value == "" || len(value) > 254 {
		return ErrPayload.WithDetail("recipient email is invalid")
	}
	parsed, err := stdmail.ParseAddress(value)
	if err != nil || !strings.EqualFold(parsed.Address, value) {
		return ErrPayload.WithDetail("recipient email is invalid")
	}
	return nil
}
