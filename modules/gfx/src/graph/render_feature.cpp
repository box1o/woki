#include <woki/gfx/graph/render_feature.hpp>

#include <algorithm>

namespace woki::gfx {
namespace {

[[nodiscard]] constexpr u64 NextRevision(const u64 revision) noexcept {
    const u64 next = revision + 1;
    return next == 0 ? 1 : next;
}

} // namespace

Result<void> RenderGraphBlackboard::Publish(const StringId name, const GraphResource resource) {
    if (name.Empty() || !resource) {
        return Err(
            ErrorCode::ValidationNullValue, "Render graph blackboard requires a name and resource");
    }
    const auto [iterator, inserted] = resources_.emplace(name, resource);
    if (!inserted && iterator->second != resource) {
        return Err(
            ErrorCode::ValidationInvalidState, "Render graph blackboard name is already published");
    }
    return Ok();
}

GraphResource RenderGraphBlackboard::Find(const StringId name) const noexcept {
    const auto iterator = resources_.find(name);
    return iterator != resources_.end() ? iterator->second : GraphResource{};
}

bool RenderGraphBlackboard::Contains(const StringId name) const noexcept {
    return resources_.contains(name);
}

std::size_t RenderGraphBlackboard::Size() const noexcept { return resources_.size(); }
void RenderGraphBlackboard::Clear() noexcept { resources_.clear(); }

auto RenderFeatureRegistry::FindEntry(const std::string_view name) noexcept {
    return std::ranges::find_if(
        features_, [name](const Entry& entry) { return entry.feature->Name() == name; });
}

auto RenderFeatureRegistry::FindEntry(const std::string_view name) const noexcept {
    return std::ranges::find_if(
        features_, [name](const Entry& entry) { return entry.feature->Name() == name; });
}

Result<void> RenderFeatureRegistry::Add(std::unique_ptr<RenderFeature> feature) {
    if (!feature || feature->Name().empty()) {
        return Err(ErrorCode::ValidationNullValue, "Render feature requires a name");
    }
    if (FindEntry(feature->Name()) != features_.end()) {
        return Err(ErrorCode::ValidationInvalidState, "Render feature name is already registered");
    }
    features_.push_back({.feature = std::move(feature)});
    revision_ = NextRevision(revision_);
    return Ok();
}

std::unique_ptr<RenderFeature> RenderFeatureRegistry::Remove(const std::string_view name) {
    const auto iterator = FindEntry(name);
    if (iterator == features_.end()) {
        return {};
    }
    auto feature = std::move(iterator->feature);
    features_.erase(iterator);
    revision_ = NextRevision(revision_);
    return feature;
}

RenderFeature* RenderFeatureRegistry::Find(const std::string_view name) noexcept {
    const auto iterator = FindEntry(name);
    return iterator != features_.end() ? iterator->feature.get() : nullptr;
}

const RenderFeature* RenderFeatureRegistry::Find(const std::string_view name) const noexcept {
    const auto iterator = FindEntry(name);
    return iterator != features_.end() ? iterator->feature.get() : nullptr;
}

Result<void> RenderFeatureRegistry::SetEnabled(const std::string_view name, const bool enabled) {
    const auto iterator = FindEntry(name);
    if (iterator == features_.end()) {
        return Err(ErrorCode::FailedToAcquireResource, "Render feature is not registered");
    }
    if (iterator->enabled != enabled) {
        iterator->enabled = enabled;
        revision_ = NextRevision(revision_);
    }
    return Ok();
}

bool RenderFeatureRegistry::IsEnabled(const std::string_view name) const noexcept {
    const auto iterator = FindEntry(name);
    return iterator != features_.end() && iterator->enabled;
}

u64 RenderFeatureRegistry::Revision() const noexcept { return revision_; }

Result<RenderFeatureGraph> RenderFeatureRegistry::Build(const RenderFeatureContext& context) {
    RenderFeatureGraph result{};
    for (auto& entry : features_) {
        if (!entry.enabled) {
            continue;
        }
        if (auto status = entry.feature->AddPasses(result.graph, result.blackboard, context);
            !status) {
            return Err(status.error());
        }
    }
    auto compiled = result.graph.Compile();
    if (!compiled) {
        return Err(compiled.error());
    }
    result.compiled = std::move(*compiled);
    return Ok(std::move(result));
}

std::size_t RenderFeatureRegistry::Size() const noexcept { return features_.size(); }
void RenderFeatureRegistry::Clear() noexcept {
    if (!features_.empty()) {
        features_.clear();
        revision_ = NextRevision(revision_);
    }
}

} // namespace woki::gfx
