package credentials

import (
	"context"
	"os"
	"path/filepath"
	"testing"
	"time"

	"github.com/box1o/woki/pkg/api"
)

func TestFileStorePermissionsAndLifecycle(t *testing.T) {
	path := filepath.Join(t.TempDir(), "nested", "credentials.json")
	s := NewFile(path)
	want := Credential{AccessToken: "secret", TokenType: "Bearer", ExpiresAt: time.Now().Add(time.Hour), Owner: api.User{ID: "u", Email: "u@example.com", Name: "U"}}
	if err := s.Save(context.Background(), want); err != nil {
		t.Fatal(err)
	}
	info, err := os.Stat(path)
	if err != nil {
		t.Fatal(err)
	}
	if info.Mode().Perm() != 0o600 {
		t.Fatalf("file mode=%o", info.Mode().Perm())
	}
	dirInfo, err := os.Stat(filepath.Dir(path))
	if err != nil {
		t.Fatal(err)
	}
	if dirInfo.Mode().Perm() != 0o700 {
		t.Fatalf("dir mode=%o", dirInfo.Mode().Perm())
	}
	got, err := s.Load(context.Background())
	if err != nil {
		t.Fatal(err)
	}
	if got.AccessToken != want.AccessToken {
		t.Fatalf("token=%q", got.AccessToken)
	}
	if err := s.Delete(context.Background()); err != nil {
		t.Fatal(err)
	}
	if _, err := s.Load(context.Background()); err == nil {
		t.Fatal("credential still loads after delete")
	}
}
func TestExpiredCredentialIsRemoved(t *testing.T) {
	path := filepath.Join(t.TempDir(), "credentials.json")
	s := NewFile(path)
	_ = os.WriteFile(path, []byte(`{"access_token":"old","expires_at":"2000-01-01T00:00:00Z"}`), 0o600)
	if _, err := s.Load(context.Background()); err == nil {
		t.Fatal("expired credential loaded")
	}
	if _, err := os.Stat(path); !os.IsNotExist(err) {
		t.Fatalf("expired credential file still exists: %v", err)
	}
}
