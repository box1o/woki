package user

import (
	"github.com/box1o/woki/internal/domain/events"
	"time"
)

const AccountCreatedEvent events.Type = "user.account_created"

type AccountCreated struct {
	UserID    string
	UserName  string
	UserEmail string
	at        time.Time
}

func NewAccountCreated(u *User) AccountCreated {
	return AccountCreated{UserID: u.ID.String(), UserName: u.Name, UserEmail: u.Email, at: time.Now().UTC()}
}
func (e AccountCreated) Type() events.Type     { return AccountCreatedEvent }
func (e AccountCreated) OccurredAt() time.Time { return e.at }
func (e AccountCreated) Payload() any          { return e }
