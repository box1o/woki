package workspace

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"time"
)

const maxSelectionFileSize = 1 << 20

type Current struct {
	ID        string    `json:"id"`
	Name      string    `json:"name"`
	UpdatedAt time.Time `json:"updated_at"`
}

type CurrentStore interface {
	Load(context.Context, string) (Current, error)
	Save(context.Context, string, Current) error
	Delete(context.Context, string) error
}

type FileCurrentStore struct {
	path string
}

type selectionFile struct {
	Current map[string]Current `json:"current_workspace,omitempty"`
}

func NewCurrentStore() (*FileCurrentStore, error) {
	dir := strings.TrimSpace(os.Getenv("WOKI_CONFIG_DIR"))
	if dir == "" {
		base, err := os.UserConfigDir()
		if err != nil {
			return nil, ErrSelectionRead.WithErr(err)
		}
		dir = filepath.Join(base, "woki")
	}
	return &FileCurrentStore{path: filepath.Join(dir, "config.json")}, nil
}

func (s *FileCurrentStore) Load(ctx context.Context, apiURL string) (Current, error) {
	if err := ctx.Err(); err != nil {
		return Current{}, err
	}
	state, err := s.read()
	if err != nil {
		return Current{}, err
	}
	current, ok := state.Current[apiKey(apiURL)]
	if !ok || strings.TrimSpace(current.ID) == "" || strings.TrimSpace(current.Name) == "" {
		return Current{}, ErrCurrentNotSet
	}
	return current, nil
}

func (s *FileCurrentStore) Save(ctx context.Context, apiURL string, current Current) error {
	if err := ctx.Err(); err != nil {
		return err
	}
	current.ID = strings.TrimSpace(current.ID)
	current.Name = strings.TrimSpace(current.Name)
	if current.ID == "" || current.Name == "" {
		return ErrSelectionInvalid
	}
	current.UpdatedAt = time.Now().UTC()
	state, err := s.read()
	if err != nil && !errors.Is(err, ErrCurrentNotSet) {
		return err
	}
	if state.Current == nil {
		state.Current = make(map[string]Current)
	}
	state.Current[apiKey(apiURL)] = current
	return s.write(state)
}

func (s *FileCurrentStore) Delete(ctx context.Context, apiURL string) error {
	if err := ctx.Err(); err != nil {
		return err
	}
	state, err := s.read()
	if err != nil {
		if errors.Is(err, ErrCurrentNotSet) {
			return nil
		}
		return err
	}
	delete(state.Current, apiKey(apiURL))
	return s.write(state)
}

func (s *FileCurrentStore) read() (selectionFile, error) {
	state := selectionFile{Current: make(map[string]Current)}
	file, err := os.Open(s.path)
	if errors.Is(err, os.ErrNotExist) {
		return state, nil
	}
	if err != nil {
		return selectionFile{}, ErrSelectionRead.WithErr(err)
	}
	defer file.Close()
	info, err := file.Stat()
	if err != nil {
		return selectionFile{}, ErrSelectionRead.WithErr(err)
	}
	if info.Size() > maxSelectionFileSize {
		return selectionFile{}, ErrSelectionRead.WithDetail("configuration file is too large")
	}
	if err := json.NewDecoder(io.LimitReader(file, maxSelectionFileSize+1)).Decode(&state); err != nil {
		return selectionFile{}, ErrSelectionRead.WithErr(err)
	}
	if state.Current == nil {
		state.Current = make(map[string]Current)
	}
	return state, nil
}

func (s *FileCurrentStore) write(state selectionFile) error {
	dir := filepath.Dir(s.path)
	if err := os.MkdirAll(dir, 0o700); err != nil {
		return ErrSelectionWrite.WithErr(err)
	}
	if err := os.Chmod(dir, 0o700); err != nil && !errors.Is(err, os.ErrPermission) {
		return ErrSelectionWrite.WithErr(err)
	}
	data, err := json.MarshalIndent(state, "", "  ")
	if err != nil {
		return ErrSelectionWrite.WithErr(err)
	}
	data = append(data, '\n')
	tmp, err := os.CreateTemp(dir, ".config-*")
	if err != nil {
		return ErrSelectionWrite.WithErr(err)
	}
	tmpName := tmp.Name()
	defer os.Remove(tmpName)
	if err := tmp.Chmod(0o600); err != nil {
		_ = tmp.Close()
		return ErrSelectionWrite.WithErr(err)
	}
	if _, err := tmp.Write(data); err != nil {
		_ = tmp.Close()
		return ErrSelectionWrite.WithErr(err)
	}
	if err := tmp.Sync(); err != nil {
		_ = tmp.Close()
		return ErrSelectionWrite.WithErr(err)
	}
	if err := tmp.Close(); err != nil {
		return ErrSelectionWrite.WithErr(err)
	}
	if err := os.Rename(tmpName, s.path); err != nil {
		return ErrSelectionWrite.WithErr(err)
	}
	if err := os.Chmod(s.path, 0o600); err != nil && !errors.Is(err, os.ErrPermission) {
		return ErrSelectionWrite.WithErr(err)
	}
	if dirFile, err := os.Open(dir); err == nil {
		_ = dirFile.Sync()
		_ = dirFile.Close()
	}
	return nil
}

func apiKey(apiURL string) string {
	return strings.TrimRight(strings.ToLower(strings.TrimSpace(apiURL)), "/")
}

func (c Current) String() string {
	return fmt.Sprintf("%s (%s)", c.Name, c.ID)
}
