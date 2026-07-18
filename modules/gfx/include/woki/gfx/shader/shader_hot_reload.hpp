#pragma once

#include "shader_dependency_graph.hpp"
#include "shader_file_watcher.hpp"

#include <span>
#include <unordered_map>
#include <vector>

namespace woki::gfx {

struct ShaderReloadBatch final {
    std::vector<ShaderFileChange> changes{};
    std::vector<ShaderHandle> shaders{};

    [[nodiscard]] bool Empty() const noexcept { return changes.empty(); }
};

class ShaderHotReload final {
public:
    [[nodiscard]] Result<void> Track(ShaderHandle shader, const paths::Path& source_path,
        std::span<const std::string> dependencies = {});

    [[nodiscard]] bool Untrack(ShaderHandle shader);
    [[nodiscard]] bool IsTracked(ShaderHandle shader) const noexcept;

    [[nodiscard]] ShaderReloadBatch Poll();

    [[nodiscard]] const ShaderDependencyGraph& Dependencies() const noexcept;
    [[nodiscard]] std::size_t ShaderCount() const noexcept;
    [[nodiscard]] std::size_t WatchedFileCount() const noexcept;

    void Clear();

private:
    struct TrackedShader final {
        std::vector<paths::Path> files{};
    };

    void AddFileReference(const paths::Path& path);
    void RemoveFileReference(const paths::Path& path);

    ShaderDependencyGraph dependencies_{};
    ShaderFileWatcher watcher_{};
    std::unordered_map<ShaderHandle, TrackedShader> shaders_{};
    std::unordered_map<paths::Path, std::size_t> file_references_{};
};

} // namespace woki::gfx
