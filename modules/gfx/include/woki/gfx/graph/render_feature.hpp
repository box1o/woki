#pragma once

#include "../scene/render_queue.hpp"
#include "render_graph.hpp"

#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace woki::gfx {

struct RenderView final {
    math::mat4f view_projection{math::mat4f::identity()};
    math::vec3f world_position{};
};

class RenderGraphBlackboard final {
public:
    [[nodiscard]] Result<void> Publish(StringId name, GraphResource resource);
    [[nodiscard]] GraphResource Find(StringId name) const noexcept;
    [[nodiscard]] bool Contains(StringId name) const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept;
    void Clear() noexcept;

private:
    std::unordered_map<StringId, GraphResource> resources_{};
};

struct RenderFeatureContext final {
    const RenderView& view;
    const RenderSnapshot& snapshot;
    const RenderQueue& opaque_queue;
    const RenderQueue& transparent_queue;
};

class RenderFeature {
public:
    virtual ~RenderFeature() = default;

    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual Result<void> AddPasses(RenderGraph& graph,
        RenderGraphBlackboard& blackboard, const RenderFeatureContext& context) = 0;
};

struct RenderFeatureGraph final {
    RenderGraph graph{};
    RenderGraphBlackboard blackboard{};
    CompiledRenderGraph compiled{};
};

class RenderFeatureRegistry final {
public:
    [[nodiscard]] Result<void> Add(std::unique_ptr<RenderFeature> feature);
    [[nodiscard]] std::unique_ptr<RenderFeature> Remove(std::string_view name);
    [[nodiscard]] RenderFeature* Find(std::string_view name) noexcept;
    [[nodiscard]] const RenderFeature* Find(std::string_view name) const noexcept;
    [[nodiscard]] Result<void> SetEnabled(std::string_view name, bool enabled);
    [[nodiscard]] bool IsEnabled(std::string_view name) const noexcept;
    [[nodiscard]] u64 Revision() const noexcept;

    [[nodiscard]] Result<RenderFeatureGraph> Build(const RenderFeatureContext& context);

    [[nodiscard]] std::size_t Size() const noexcept;
    void Clear() noexcept;

private:
    struct Entry final {
        std::unique_ptr<RenderFeature> feature{};
        bool enabled{true};
    };

    [[nodiscard]] auto FindEntry(std::string_view name) noexcept;
    [[nodiscard]] auto FindEntry(std::string_view name) const noexcept;

    std::vector<Entry> features_{};
    u64 revision_{1};
};

} // namespace woki::gfx
