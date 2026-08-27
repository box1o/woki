package auth

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/box1o/woki/internal/domain/user"
	"github.com/box1o/woki/internal/infrastructure/provider"
	authsvc "github.com/box1o/woki/internal/services/auth"
	"github.com/box1o/woki/pkg/config"
)

type fakeGoogle struct{}

func (fakeGoogle) Configured() bool { return true }
func (fakeGoogle) AuthURL(state string) string {
	return "https://accounts.google.example/auth?state=" + state
}
func (fakeGoogle) Exchange(context.Context, string) (provider.Profile, error) {
	return provider.Profile{
		Email:      "user@example.com",
		Name:       "User",
		Provider:   user.ProviderGoogle,
		ProviderID: "123",
	}, nil
}

type fakeService struct{}

func (fakeService) Login(context.Context, authsvc.Profile) (*user.User, string, error) {
	usr, err := user.New(
		"user@example.com",
		"User",
		"",
		user.ProviderGoogle,
		"123",
	)
	return usr, "session-token", err
}
func (fakeService) Logout(context.Context, string) error { return nil }

func TestSafeReturnPath(t *testing.T) {
	for name, tc := range map[string]struct {
		input string
		want  string
	}{
		"root":              {"/", "/"},
		"device":            {"/device?code=ABCDEFGH", "/device?code=ABCDEFGH"},
		"absolute":          {"https://evil.example/", "/"},
		"scheme-relative":   {"//evil.example/", "/"},
		"backslash":         {`/\evil.example`, "/"},
		"encoded-backslash": {`/%5cevil.example`, "/"},
		"fragment":          {"/device#secret", "/"},
	} {
		t.Run(name, func(t *testing.T) {
			if got := safeReturnPath(tc.input); got != tc.want {
				t.Fatalf("safeReturnPath(%q)=%q; want %q", tc.input, got, tc.want)
			}
		})
	}
}

func TestGoogleCallbackPreservesReturnPath(t *testing.T) {
	h := New(
		fakeService{},
		fakeGoogle{},
		nil,
		"http://localhost:5173",
		false,
		config.CookieConfig{Name: "woki_session", Secure: false},
		time.Hour,
	)
	req := httptest.NewRequest(
		http.MethodGet,
		"/auth/google/callback?state=state&code=code",
		nil,
	)
	req.AddCookie(&http.Cookie{Name: oauthStateCookie(), Value: "state"})
	req.AddCookie(&http.Cookie{Name: oauthReturnCookie(), Value: "/device?code=ABCDEFGH"})
	w := httptest.NewRecorder()

	h.finishGoogle(w, req)

	if w.Code != http.StatusFound {
		t.Fatalf("status=%d body=%s", w.Code, w.Body.String())
	}
	if got := w.Header().Get("Location"); got != "http://localhost:5173/device?code=ABCDEFGH" {
		t.Fatalf("Location=%q", got)
	}
}
