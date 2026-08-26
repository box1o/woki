package memory

import apperrors "github.com/box1o/woki/pkg/errors"

var (
	ErrSnapshotVersion = apperrors.New("MEMORY_SNAPSHOT_VERSION_UNSUPPORTED", "storage snapshot version is unsupported")
	ErrSnapshotInvalid = apperrors.New("MEMORY_SNAPSHOT_INVALID", "storage snapshot is invalid")
)
