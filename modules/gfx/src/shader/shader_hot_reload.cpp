#include <woki/gfx/shader/shader_hot_reload.hpp>

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>

namespace woki::gfx {

Result<void> ShaderHotReload::Track(const ShaderHandle shader, const paths::Path& source_path,
    const std::span<const std::string> dependencies) {
    if (!shader) {
        return Err(ErrorCode::InvalidArgument, "Cannot track an invalid shader handle");
    }
    if (source_path.empty()) {
        return Err(ErrorCode::InvalidArgument, "Tracked shader requires a source path");
    }

    std::vector<paths::Path> files{};
    files.reserve(dependencies.size() + 1);

    auto normalized_source = paths::Normalize(source_path);
    if (!normalized_source) {
        return Err(normalized_source.error());
    }
    const paths::Path normalized_source_path = *normalized_source;
    files.push_back(normalized_source_path);

    for (const auto& dependency : dependencies) {
        auto normalized = paths::Normalize(dependency);
        if (!normalized) {
            return Err(normalized.error());
        }
        files.push_back(std::move(*normalized));
    }

    std::ranges::sort(files);
    const auto unique_end = std::ranges::unique(files).begin();
    files.erase(unique_end, files.end());

    static_cast<void>(Untrack(shader));
    std::vector<paths::Path> added_files{};
    added_files.reserve(files.size());
    for (const auto& file : files) {
        if (!file_references_.contains(file)) {
            auto watched = watcher_.Watch(file);
            if (!watched) {
                for (const auto& added : added_files) {
                    RemoveFileReference(added);
                }
                return Err(watched.error());
            }
        }
        AddFileReference(file);
        added_files.push_back(file);
    }

    const std::string source_name = normalized_source_path.generic_string();
    std::vector<std::string> dependency_names{};
    dependency_names.reserve(files.size() - 1);
    for (const auto& file : files) {
        if (file.generic_string() != source_name) {
            dependency_names.push_back(file.generic_string());
        }
    }

    dependencies_.Update(shader, source_name, dependency_names);
    shaders_.emplace(shader, TrackedShader{.files = std::move(files)});
    return Ok();
}

bool ShaderHotReload::Untrack(const ShaderHandle shader) {
    const auto iterator = shaders_.find(shader);
    if (iterator == shaders_.end()) {
        return false;
    }

    for (const auto& file : iterator->second.files) {
        RemoveFileReference(file);
    }
    static_cast<void>(dependencies_.Remove(shader));
    shaders_.erase(iterator);
    return true;
}

bool ShaderHotReload::IsTracked(const ShaderHandle shader) const noexcept {
    return shaders_.contains(shader);
}

ShaderReloadBatch ShaderHotReload::Poll() {
    ShaderReloadBatch batch{};
    batch.changes = watcher_.Poll();
    if (batch.changes.empty()) {
        return batch;
    }

    std::vector<std::string> changed_sources{};
    changed_sources.reserve(batch.changes.size());
    for (const auto& change : batch.changes) {
        changed_sources.push_back(change.path.generic_string());
    }
    batch.shaders = dependencies_.AffectedBy(changed_sources);
    return batch;
}

const ShaderDependencyGraph& ShaderHotReload::Dependencies() const noexcept {
    return dependencies_;
}

std::size_t ShaderHotReload::ShaderCount() const noexcept { return shaders_.size(); }

std::size_t ShaderHotReload::WatchedFileCount() const noexcept {
    return watcher_.WatchedFileCount();
}

void ShaderHotReload::Clear() {
    dependencies_.Clear();
    watcher_.Clear();
    shaders_.clear();
    file_references_.clear();
}

void ShaderHotReload::AddFileReference(const paths::Path& path) { ++file_references_[path]; }

void ShaderHotReload::RemoveFileReference(const paths::Path& path) {
    const auto iterator = file_references_.find(path);
    WOKI_ASSERT(iterator != file_references_.end());
    WOKI_ASSERT(iterator->second > 0);

    --iterator->second;
    if (iterator->second == 0) {
        static_cast<void>(watcher_.Unwatch(path));
        file_references_.erase(iterator);
    }
}

} // namespace woki::gfx
