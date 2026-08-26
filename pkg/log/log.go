// Package log provides the process-wide Woki logger.
//
// The package intentionally keeps the compact Fuse logging API while making
// configuration explicit and all reconfiguration/write operations safe for
// concurrent use.
package log

import (
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

type OutputType int

const (
	Console OutputType = iota
	File
	ConsoleAndFile
)

type LevelType int

const (
	TraceLevel LevelType = iota
	DebugLevel
	InfoLevel
	WarnLevel
	ErrorLevel
)

type Config struct {
	Output        OutputType
	Level         LevelType
	FilePath      string
	DisableColors bool
}

type state struct {
	mu            sync.RWMutex
	out           io.Writer
	file          *os.File
	level         LevelType
	disableColors bool
}

var global = state{
	out:           os.Stdout,
	level:         InfoLevel,
	disableColors: os.Getenv("NO_COLOR") != "",
}

func Setup(output OutputType, filePath string) error {
	global.mu.RLock()
	level := global.level
	global.mu.RUnlock()
	return Configure(Config{
		Output:        output,
		Level:         level,
		FilePath:      filePath,
		DisableColors: os.Getenv("NO_COLOR") != "",
	})
}

func Configure(cfg Config) error {
	if !cfg.Level.Valid() {
		return ErrInvalidLevel.WithDetail(fmt.Sprint(cfg.Level))
	}
	writers, file, err := writersFor(cfg.Output, cfg.FilePath)
	if err != nil {
		return err
	}

	global.mu.Lock()
	oldFile := global.file
	global.out = io.MultiWriter(writers...)
	global.file = file
	global.level = cfg.Level
	global.disableColors = cfg.DisableColors || cfg.Output != Console || os.Getenv("NO_COLOR") != ""
	global.mu.Unlock()

	if oldFile != nil {
		if err := oldFile.Close(); err != nil {
			return ErrCleanup.WithErr(err)
		}
		return nil
	}
	return nil
}

func Cleanup() error {
	global.mu.Lock()
	file := global.file
	global.file = nil
	global.out = os.Stdout
	global.mu.Unlock()
	if file != nil {
		if err := file.Close(); err != nil {
			return ErrCleanup.WithErr(err)
		}
		return nil
	}
	return nil
}

func SetLevel(level LevelType) {
	if !level.Valid() {
		level = InfoLevel
	}
	global.mu.Lock()
	global.level = level
	global.mu.Unlock()
}

func GetLevel() LevelType {
	global.mu.RLock()
	defer global.mu.RUnlock()
	return global.level
}

func ParseLevel(value string) (LevelType, error) {
	switch strings.ToLower(strings.TrimSpace(value)) {
	case "trace":
		return TraceLevel, nil
	case "debug":
		return DebugLevel, nil
	case "info", "":
		return InfoLevel, nil
	case "warn", "warning":
		return WarnLevel, nil
	case "error":
		return ErrorLevel, nil
	default:
		return InfoLevel, ErrInvalidLevel.WithDetail(value)
	}
}

func ParseOutput(value string) (OutputType, error) {
	switch strings.ToLower(strings.TrimSpace(value)) {
	case "console", "":
		return Console, nil
	case "file":
		return File, nil
	case "both", "console-and-file", "console_and_file":
		return ConsoleAndFile, nil
	default:
		return Console, ErrInvalidOutput.WithDetail(value)
	}
}

func Trace(format string, args ...any) { logf(TraceLevel, format, args...) }
func Debug(format string, args ...any) { logf(DebugLevel, format, args...) }
func Info(format string, args ...any)  { logf(InfoLevel, format, args...) }
func Warn(format string, args ...any)  { logf(WarnLevel, format, args...) }
func Error(format string, args ...any) { logf(ErrorLevel, format, args...) }

func logf(level LevelType, format string, args ...any) {
	global.mu.RLock()
	defer global.mu.RUnlock()
	if level < global.level {
		return
	}

	message := fmt.Sprintf(format, args...)
	stamp := time.Now().Format("15:04:05")
	label := fmt.Sprintf("%-5s", level.String())
	if global.disableColors {
		_, _ = fmt.Fprintf(global.out, "[%s:%s] %s\n", label, stamp, message)
		return
	}
	_, _ = fmt.Fprintf(global.out, "%s[%s:%s]\x1b[0m %s\n", levelColor(level), label, stamp, message)
}

func writersFor(output OutputType, filePath string) ([]io.Writer, *os.File, error) {
	switch output {
	case Console:
		return []io.Writer{os.Stdout}, nil, nil
	case File, ConsoleAndFile:
		filePath = strings.TrimSpace(filePath)
		if filePath == "" {
			return nil, nil, ErrFileRequired
		}
		if err := os.MkdirAll(filepath.Dir(filePath), 0o700); err != nil && filepath.Dir(filePath) != "." {
			return nil, nil, ErrDirectory.WithErr(err)
		}
		file, err := os.OpenFile(filePath, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0o600)
		if err != nil {
			return nil, nil, ErrFileOpen.WithErr(err)
		}
		if output == File {
			return []io.Writer{file}, file, nil
		}
		return []io.Writer{os.Stdout, file}, file, nil
	default:
		return nil, nil, ErrInvalidOutput.WithDetail(fmt.Sprint(output))
	}
}

func (o OutputType) Valid() bool { return o >= Console && o <= ConsoleAndFile }

func (l LevelType) Valid() bool { return l >= TraceLevel && l <= ErrorLevel }

func (l LevelType) String() string {
	switch l {
	case TraceLevel:
		return "TRACE"
	case DebugLevel:
		return "DEBUG"
	case InfoLevel:
		return "INFO"
	case WarnLevel:
		return "WARN"
	case ErrorLevel:
		return "ERROR"
	default:
		return "UNKNOWN"
	}
}

func levelColor(level LevelType) string {
	switch level {
	case TraceLevel:
		return "\x1b[35m"
	case DebugLevel:
		return "\x1b[36m"
	case InfoLevel:
		return "\x1b[32m"
	case WarnLevel:
		return "\x1b[33m"
	case ErrorLevel:
		return "\x1b[31m"
	default:
		return "\x1b[37m"
	}
}
