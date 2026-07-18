#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include <woki/core.hpp>

namespace woki::gfx {

enum class ShaderFileChangeType : u8 {
    Created = 0,
    Modified,
    Removed,
};

struct ShaderFileChange final {
    paths::Path path{};
    ShaderFileChangeType type{ShaderFileChangeType::Modified};
};

class ShaderFileWatcher final {
public:
    [[nodiscard]] Result<void> Watch(const paths::Path& path);
    [[nodiscard]] bool Unwatch(const paths::Path& path);
    [[nodiscard]] bool IsWatching(const paths::Path& path) const;

    [[nodiscard]] std::vector<ShaderFileChange> Poll();

    [[nodiscard]] std::size_t WatchedFileCount() const noexcept;
    [[nodiscard]] bool Empty() const noexcept;

    void Clear();

private:
    struct FileState final {
        std::filesystem::file_time_type write_time{};
        std::uintmax_t size{0};
        bool exists{false};
    };

    [[nodiscard]] static FileState ReadState(const paths::Path& path) noexcept;

    std::unordered_map<paths::Path, FileState> files_{};
};

[[nodiscard]] constexpr std::string_view ToString(const ShaderFileChangeType type) noexcept {
    switch (type) {
    case ShaderFileChangeType::Created:
        return "Created";
    case ShaderFileChangeType::Modified:
        return "Modified";
    case ShaderFileChangeType::Removed:
        return "Removed";
    }

    return "Unknown";
}

} // namespace woki::gfx
