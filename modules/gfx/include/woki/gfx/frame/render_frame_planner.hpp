#pragma once

#include "../graph/render_feature.hpp"
#include "../scene/render_scene.hpp"

namespace woki::gfx {

struct RenderFramePlan final {
    RenderSnapshot snapshot{};
    RenderQueue opaque_queue{};
    RenderQueue transparent_queue{};
    RenderFeatureGraph feature_graph{};
};

[[nodiscard]] Result<RenderFramePlan> BuildRenderFramePlan(
    RenderSnapshot snapshot, RenderFeatureRegistry& features);

class RenderFramePlanner final {
public:
    RenderFramePlanner(RenderScene& scene, RenderFeatureRegistry& features) noexcept;

    [[nodiscard]] Result<RenderFramePlan> Prepare(u64 layer_mask = ~0ULL);

private:
    RenderScene* scene_{nullptr};
    RenderFeatureRegistry* features_{nullptr};
};

} // namespace woki::gfx
