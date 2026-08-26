// Package mail defines the delivery contract used by application mail services.
package mail

import "context"

type Message struct {
	To      []string
	ReplyTo string
	Subject string
	HTML    string
	Text    string
}

type Sender interface {
	Send(context.Context, Message) error
}
