package deviceauth

import (
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/box1o/woki/internal/infrastructure/memory"
	service "github.com/box1o/woki/internal/services/deviceauth"
)

func TestCreateRejectsMalformedJSON(t *testing.T) {
	h := &Handler{}
	req := httptest.NewRequest(http.MethodPost, "/auth/device/code", strings.NewReader(`{"client_name":`))
	req.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()

	h.create(w, req)

	if w.Code != http.StatusBadRequest {
		t.Fatalf("status=%d body=%s", w.Code, w.Body.String())
	}
}

func TestCreateReturnsDeviceCode(t *testing.T) {
	store := memory.New()
	svc := service.New("https://woki.example", time.Minute, time.Hour, memory.CredentialRepository{Store: store}, memory.UserRepository{Store: store})
	h := &Handler{service: svc}
	req := httptest.NewRequest(http.MethodPost, "/auth/device/code", strings.NewReader(`{"client_name":"Woki CLI"}`))
	req.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()

	h.create(w, req)

	if w.Code != http.StatusOK {
		t.Fatalf("status=%d body=%s", w.Code, w.Body.String())
	}
	if !strings.Contains(w.Body.String(), `"user_code"`) || !strings.Contains(w.Body.String(), `"device_code"`) {
		t.Fatalf("unexpected response: %s", w.Body.String())
	}
}
