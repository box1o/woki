package mail

import (
	"context"
	"strings"

	domainmail "github.com/box1o/woki/internal/domain/mail"
	"github.com/box1o/woki/pkg/config"
	gomail "github.com/wneessen/go-mail"
)

type SMTP struct {
	client   *gomail.Client
	from     string
	fromName string
}

var _ domainmail.Sender = (*SMTP)(nil)

func NewSMTP(cfg config.MailConfig) (*SMTP, error) {
	opts := []gomail.Option{
		gomail.WithPort(cfg.Port),
		gomail.WithUsername(cfg.From),
		gomail.WithPassword(cfg.Password),
		gomail.WithSMTPAuth(gomail.SMTPAuthPlain),
		gomail.WithTLSPolicy(gomail.TLSMandatory),
		gomail.WithTimeout(cfg.SendTimeout),
	}
	client, err := gomail.NewClient(cfg.Host, opts...)
	if err != nil {
		return nil, ErrConfigure.WithErr(err)
	}
	return &SMTP{
		client:   client,
		from:     cfg.From,
		fromName: strings.TrimSpace(cfg.Name),
	}, nil
}

func (s *SMTP) Send(ctx context.Context, message domainmail.Message) error {
	if err := ctx.Err(); err != nil {
		return ErrSend.WithErr(err)
	}

	msg := gomail.NewMsg()
	var err error
	if s.fromName == "" {
		err = msg.From(s.from)
	} else {
		err = msg.FromFormat(s.fromName, s.from)
	}
	if err != nil {
		return ErrSend.WithErr(err)
	}
	if err := msg.To(message.To...); err != nil {
		return ErrSend.WithErr(err)
	}
	if message.ReplyTo != "" {
		if err := msg.SetAddrHeader(gomail.HeaderReplyTo, message.ReplyTo); err != nil {
			return ErrSend.WithErr(err)
		}
	}
	msg.Subject(message.Subject)
	msg.SetDate()
	msg.SetMessageID()
	if message.Text != "" {
		msg.SetBodyString(gomail.TypeTextPlain, message.Text)
	}
	if message.HTML != "" {
		msg.SetBodyString(gomail.TypeTextHTML, message.HTML)
	}
	if message.Text == "" && message.HTML == "" {
		msg.SetBodyString(gomail.TypeTextPlain, "")
	}

	if err := s.client.DialAndSendWithContext(ctx, msg); err != nil {
		return ErrSend.WithErr(err)
	}
	return nil
}
