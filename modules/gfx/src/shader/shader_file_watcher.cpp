#include <woki/gfx/shader/shader_file_watcher.hpp>

#include <algorithm>
#include <system_error>

namespace woki::gfx {

Result<void> ShaderFileWatcher::Watch(const paths::Path& path) {
    if (path.empty()) {
        return Err(ErrorCode::InvalidArgument, "Shader watch path cannot be empty");
    }

    auto normalized = paths::Normalize(path);
    if (!normalized) {
        return Err(normalized.error());
    }

    files_.insert_or_assign(*normalized, ReadState(*normalized));
    return Ok();
}

bool ShaderFileWatcher::Unwatch(const paths::Path& path) {
    if (path.empty()) {
        return false;
    }
    const auto normalized = paths::Normalize(path);
    return normalized && files_.erase(*normalized) != 0;
}

bool ShaderFileWatcher::IsWatching(const paths::Path& path) const {
    if (path.empty()) {
        return false;
    }
    const auto normalized = paths::Normalize(path);
    return normalized && files_.contains(*normalized);
}

std::vector<ShaderFileChange> ShaderFileWatcher::Poll() {
    std::vector<ShaderFileChange> changes{};
    changes.reserve(files_.size());

    for (auto& [path, previous] : files_) {
        const FileState current = ReadState(path);
        if (!previous.exists && current.exists) {
            changes.push_back({.path = path, .type = ShaderFileChangeType::Created});
        } else if (previous.exists && !current.exists) {
            changes.push_back({.path = path, .type = ShaderFileChangeType::Removed});
        } else if (current.exists &&
                   (current.write_time != previous.write_time || current.size != previous.size)) {
            changes.push_back({.path = path, .type = ShaderFileChangeType::Modified});
        }
        previous = current;
    }

    std::ranges::sort(
        changes, {}, [](const ShaderFileChange& change) { return change.path.generic_string(); });
    return changes;
}

std::size_t ShaderFileWatcher::WatchedFileCount() const noexcept { return files_.size(); }

bool ShaderFileWatcher::Empty() const noexcept { return files_.empty(); }

void ShaderFileWatcher::Clear() { files_.clear(); }

ShaderFileWatcher::FileState ShaderFileWatcher::ReadState(const paths::Path& path) noexcept {
    std::error_code error;
    const bool exists = std::filesystem::is_regular_file(path, error);
    if (error || !exists) {
        return {};
    }

    const auto write_time = std::filesystem::last_write_time(path, error);
    if (error) {
        return {};
    }

    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        return {};
    }

    return {
        .write_time = write_time,
        .size = size,
        .exists = true,
    };
}

} // namespace woki::gfx
