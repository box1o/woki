package platform

import (
	"os/exec"
	"runtime"
)

type Browser struct{}

func (Browser) Open(url string) error {
	var cmd *exec.Cmd
	switch runtime.GOOS {
	case "linux":
		cmd = exec.Command("xdg-open", url)
	case "darwin":
		cmd = exec.Command("open", url)
	case "windows":
		cmd = exec.Command("rundll32", "url.dll,FileProtocolHandler", url)
	default:
		return ErrBrowserUnsupported.WithDetail(runtime.GOOS)
	}
	if err := cmd.Start(); err != nil {
		return ErrBrowserOpen.WithErr(err)
	}
	return nil
}
