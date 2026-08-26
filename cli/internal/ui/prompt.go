package ui

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

type Option struct {
	Label       string
	Description string
}

func CanPrompt(in io.Reader) bool {
	file, ok := in.(*os.File)
	if !ok {
		return true
	}
	return isTerminal(file)
}

func PromptLine(in io.Reader, out io.Writer, label string) (string, error) {
	if _, err := fmt.Fprintf(out, "%s: ", strings.TrimSpace(label)); err != nil {
		return "", err
	}
	value, err := bufio.NewReader(in).ReadString('\n')
	if err != nil && err != io.EOF {
		return "", err
	}
	return strings.TrimSpace(value), nil
}

func Select(in io.Reader, out io.Writer, title string, options []Option, current int) (int, error) {
	if len(options) == 0 {
		return -1, ErrSelectionEmpty
	}
	if _, err := fmt.Fprintln(out, strings.TrimSpace(title)); err != nil {
		return -1, err
	}
	for i, option := range options {
		marker := " "
		if i == current {
			marker = "*"
		}
		if option.Description == "" {
			if _, err := fmt.Fprintf(out, " %s %d) %s\n", marker, i+1, option.Label); err != nil {
				return -1, err
			}
			continue
		}
		if _, err := fmt.Fprintf(out, " %s %d) %s  %s\n", marker, i+1, option.Label, option.Description); err != nil {
			return -1, err
		}
	}
	value, err := PromptLine(in, out, "Select")
	if err != nil {
		return -1, err
	}
	index, err := strconv.Atoi(value)
	if err != nil || index < 1 || index > len(options) {
		return -1, ErrSelectionInvalid.WithDetail(value)
	}
	return index - 1, nil
}
