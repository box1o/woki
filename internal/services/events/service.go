package events

import (
	"context"
	domain "github.com/box1o/woki/internal/domain/events"
)

type Service struct{ bus domain.Bus }

func New(bus domain.Bus) *Service  { return &Service{bus: bus} }
func (s *Service) Bus() domain.Bus { return s.bus }
func (s *Service) Publish(ctx context.Context, event domain.Event) error {
	return s.bus.Publish(ctx, event)
}
