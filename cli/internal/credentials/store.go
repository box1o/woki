// Package credentials persists the local CLI bearer credential.
package credentials

import (
	"bytes"
	"context"
	"encoding/json"
	stderrors "errors"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"

	"github.com/box1o/woki/pkg/api"
)

type Credential struct {
	AccessToken string    `json:"access_token"`
	TokenType   string    `json:"token_type"`
	ExpiresAt   time.Time `json:"expires_at"`
	Owner       api.User  `json:"owner"`
}

func (c Credential) Valid(now time.Time) bool {
	return strings.TrimSpace(c.AccessToken) != "" && now.Before(c.ExpiresAt)
}

type Store struct {
	path       string
	secretTool string
	useKeyring bool
}

func New() (*Store, error) {
	dir, err := os.UserConfigDir()
	if err != nil {
		return nil, ErrConfigDir.WithErr(err)
	}
	s := &Store{path: filepath.Join(dir, "woki", "credentials.json")}
	if runtime.GOOS == "linux" && strings.TrimSpace(os.Getenv("DBUS_SESSION_BUS_ADDRESS")) != "" {
		if tool, err := exec.LookPath("secret-tool"); err == nil {
			s.secretTool = tool
			s.useKeyring = true
		}
	}
	return s, nil
}
func NewFile(path string) *Store { return &Store{path: path} }
func (s *Store) Save(ctx context.Context, c Credential) error {
	if !c.Valid(time.Now()) {
		return ErrInvalid
	}
	if s.useKeyring {
		if err := s.saveKeyring(ctx, c); err == nil {
			if removeErr := os.Remove(s.path); removeErr != nil && !stderrors.Is(removeErr, os.ErrNotExist) {
				cleanupErr := s.deleteKeyring(ctx)
				return stderrors.Join(ErrDelete.WithErr(removeErr), cleanupErr)
			}
			return nil
		}
	}
	return s.saveFile(c)
}
func (s *Store) Load(ctx context.Context) (Credential, error) {
	if s.useKeyring {
		c, err := s.loadKeyring(ctx)
		if err == nil {
			if !c.Valid(time.Now()) {
				if deleteErr := s.Delete(ctx); deleteErr != nil {
					return Credential{}, stderrors.Join(ErrNotFound, deleteErr)
				}
				return Credential{}, ErrNotFound
			}
			return c, nil
		}
		if !stderrors.Is(err, ErrNotFound) {
			return Credential{}, err
		}
	}
	c, err := s.loadFile()
	if err != nil {
		return Credential{}, err
	}
	if !c.Valid(time.Now()) {
		if deleteErr := s.Delete(ctx); deleteErr != nil {
			return Credential{}, stderrors.Join(ErrNotFound, deleteErr)
		}
		return Credential{}, ErrNotFound
	}
	return c, nil
}
func (s *Store) Delete(ctx context.Context) error {
	var errs []error
	if s.useKeyring {
		if err := s.deleteKeyring(ctx); err != nil {
			errs = append(errs, err)
		}
	}
	if err := os.Remove(s.path); err != nil && !stderrors.Is(err, os.ErrNotExist) {
		errs = append(errs, ErrDelete.WithErr(err))
	}
	if len(errs) > 0 {
		return stderrors.Join(errs...)
	}
	return nil
}

func (s *Store) saveFile(c Credential) error {
	if strings.TrimSpace(s.path) == "" || filepath.Clean(s.path) == "." {
		return ErrWrite.WithDetail("credential file path is invalid")
	}
	dir := filepath.Dir(s.path)
	if err := os.MkdirAll(dir, 0o700); err != nil {
		return ErrWrite.WithDetail("create credential directory").WithErr(err)
	}
	if runtime.GOOS != "windows" {
		if err := os.Chmod(dir, 0o700); err != nil {
			return ErrFilePermission.WithDetail("credential directory").WithErr(err)
		}
	}
	data, err := json.MarshalIndent(c, "", "  ")
	if err != nil {
		return ErrWrite.WithDetail("encode credential").WithErr(err)
	}
	data = append(data, '\n')

	tmp, err := os.CreateTemp(dir, ".credentials-*")
	if err != nil {
		return ErrWrite.WithDetail("create temporary credential file").WithErr(err)
	}
	name := tmp.Name()
	cleanup := func() {
		_ = tmp.Close()
		_ = os.Remove(name)
	}
	if runtime.GOOS != "windows" {
		if err := tmp.Chmod(0o600); err != nil {
			cleanup()
			return ErrFilePermission.WithDetail("temporary credential file").WithErr(err)
		}
	}
	if _, err := tmp.Write(data); err != nil {
		cleanup()
		return ErrWrite.WithDetail("write credential").WithErr(err)
	}
	if err := tmp.Sync(); err != nil {
		cleanup()
		return ErrWrite.WithDetail("sync credential file").WithErr(err)
	}
	if err := tmp.Close(); err != nil {
		_ = os.Remove(name)
		return ErrWrite.WithDetail("close credential file").WithErr(err)
	}
	if err := os.Rename(name, s.path); err != nil {
		_ = os.Remove(name)
		return ErrWrite.WithDetail("replace credential file").WithErr(err)
	}
	if runtime.GOOS != "windows" {
		if err := os.Chmod(s.path, 0o600); err != nil {
			return ErrFilePermission.WithDetail("credential file").WithErr(err)
		}
		if err := syncDirectory(dir); err != nil {
			return ErrWrite.WithDetail("sync credential directory").WithErr(err)
		}
	}
	return nil
}

func (s *Store) loadFile() (Credential, error) {
	info, err := os.Stat(s.path)
	if stderrors.Is(err, os.ErrNotExist) {
		return Credential{}, ErrNotFound
	}
	if err != nil {
		return Credential{}, ErrRead.WithErr(err)
	}
	if info.Size() > 64<<10 {
		return Credential{}, ErrFileTooLarge
	}
	if runtime.GOOS != "windows" && info.Mode().Perm()&0o077 != 0 {
		if err := os.Chmod(s.path, 0o600); err != nil {
			return Credential{}, ErrFilePermission.WithErr(err)
		}
	}
	data, err := os.ReadFile(s.path)
	if err != nil {
		return Credential{}, ErrRead.WithErr(err)
	}
	var c Credential
	if err := json.Unmarshal(data, &c); err != nil {
		return Credential{}, ErrDecode.WithErr(err)
	}
	return c, nil
}

func syncDirectory(dir string) error {
	fd, err := os.Open(dir)
	if err != nil {
		return err
	}
	if err := fd.Sync(); err != nil {
		_ = fd.Close()
		return err
	}
	return fd.Close()
}

func (s *Store) keyringContext(parent context.Context) (context.Context, context.CancelFunc) {
	if parent == nil {
		parent = context.Background()
	}
	return context.WithTimeout(parent, 3*time.Second)
}
func (s *Store) saveKeyring(parent context.Context, c Credential) error {
	ctx, cancel := s.keyringContext(parent)
	defer cancel()
	data, err := json.Marshal(c)
	if err != nil {
		return err
	}
	cmd := exec.CommandContext(ctx, s.secretTool, "store", "--label=Woki CLI", "service", "woki", "account", "default")
	cmd.Stdin = bytes.NewReader(data)
	if out, err := cmd.CombinedOutput(); err != nil {
		return ErrKeyring.WithDetail(strings.TrimSpace(string(out))).WithErr(err)
	}
	return nil
}
func (s *Store) loadKeyring(parent context.Context) (Credential, error) {
	ctx, cancel := s.keyringContext(parent)
	defer cancel()
	cmd := exec.CommandContext(ctx, s.secretTool, "lookup", "service", "woki", "account", "default")
	out, err := cmd.Output()
	if err != nil {
		if ctx.Err() != nil {
			return Credential{}, ErrKeyring.WithErr(ctx.Err())
		}
		var exitErr *exec.ExitError
		if stderrors.As(err, &exitErr) && exitErr.ExitCode() == 1 {
			return Credential{}, ErrNotFound
		}
		return Credential{}, ErrKeyring.WithErr(err)
	}
	var c Credential
	if err := json.Unmarshal(bytes.TrimSpace(out), &c); err != nil {
		return Credential{}, ErrDecode.WithErr(err)
	}
	return c, nil
}
func (s *Store) deleteKeyring(parent context.Context) error {
	ctx, cancel := s.keyringContext(parent)
	defer cancel()
	cmd := exec.CommandContext(ctx, s.secretTool, "clear", "service", "woki", "account", "default")
	if out, err := cmd.CombinedOutput(); err != nil {
		return ErrKeyring.WithDetail(strings.TrimSpace(string(out))).WithErr(err)
	}
	return nil
}
