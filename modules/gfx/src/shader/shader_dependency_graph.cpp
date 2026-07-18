#include <woki/gfx/shader/shader_dependency_graph.hpp>

#include <algorithm>
#include <utility>

namespace woki::gfx {

void ShaderDependencyGraph::Update(const ShaderHandle shader, std::string source_name,
    const std::span<const std::string> dependencies) {
    WOKI_ASSERT(shader.Valid());

    static_cast<void>(Remove(shader));

    ShaderDependencies record{
        .source_name = std::move(source_name),
        .dependencies = {dependencies.begin(), dependencies.end()},
    };

    std::ranges::sort(record.dependencies);
    const auto unique_end = std::ranges::unique(record.dependencies).begin();
    record.dependencies.erase(unique_end, record.dependencies.end());

    AddReverseEdge(record.source_name, shader);
    for (const auto& dependency : record.dependencies) {
        AddReverseEdge(dependency, shader);
    }

    shaders_.emplace(shader, std::move(record));
}

bool ShaderDependencyGraph::Remove(const ShaderHandle shader) {
    const auto iterator = shaders_.find(shader);
    if (iterator == shaders_.end()) {
        return false;
    }

    RemoveReverseEdge(iterator->second.source_name, shader);
    for (const auto& dependency : iterator->second.dependencies) {
        RemoveReverseEdge(dependency, shader);
    }
    shaders_.erase(iterator);
    return true;
}

std::vector<ShaderHandle> ShaderDependencyGraph::AffectedBy(
    const std::string_view changed_source) const {
    const auto iterator = dependents_.find(std::string(changed_source));
    if (iterator == dependents_.end()) {
        return {};
    }

    std::vector<ShaderHandle> affected{iterator->second.begin(), iterator->second.end()};
    std::ranges::sort(affected, {}, &ShaderHandle::Value);
    return affected;
}

std::vector<ShaderHandle> ShaderDependencyGraph::AffectedBy(
    const std::span<const std::string> changed_sources) const {
    std::unordered_set<ShaderHandle> unique{};
    for (const auto& source : changed_sources) {
        const auto affected = AffectedBy(source);
        unique.insert(affected.begin(), affected.end());
    }

    std::vector<ShaderHandle> result{unique.begin(), unique.end()};
    std::ranges::sort(result, {}, &ShaderHandle::Value);
    return result;
}

std::span<const std::string> ShaderDependencyGraph::Dependencies(
    const ShaderHandle shader) const noexcept {
    const auto iterator = shaders_.find(shader);
    if (iterator == shaders_.end()) {
        return {};
    }
    return iterator->second.dependencies;
}

std::size_t ShaderDependencyGraph::ShaderCount() const noexcept { return shaders_.size(); }

bool ShaderDependencyGraph::Empty() const noexcept { return shaders_.empty(); }

void ShaderDependencyGraph::Clear() {
    shaders_.clear();
    dependents_.clear();
}

void ShaderDependencyGraph::AddReverseEdge(
    const std::string_view source_name, const ShaderHandle shader) {
    if (!source_name.empty()) {
        dependents_[std::string(source_name)].insert(shader);
    }
}

void ShaderDependencyGraph::RemoveReverseEdge(
    const std::string_view source_name, const ShaderHandle shader) {
    if (source_name.empty()) {
        return;
    }

    const auto iterator = dependents_.find(std::string(source_name));
    if (iterator == dependents_.end()) {
        return;
    }

    iterator->second.erase(shader);
    if (iterator->second.empty()) {
        dependents_.erase(iterator);
    }
}

} // namespace woki::gfx
