#include <woki/gfx/frame/render_frame_planner.hpp>

namespace woki::gfx {

Result<RenderFramePlan> BuildRenderFramePlan(
    RenderSnapshot snapshot, RenderFeatureRegistry& features, const RenderView& view) {
    auto opaque = BuildRenderQueue(snapshot, {.phase = DrawPhase::Opaque});
    if (!opaque) {
        return Err(opaque.error());
    }
    auto transparent = BuildRenderQueue(snapshot, {.phase = DrawPhase::Transparent});
    if (!transparent) {
        return Err(transparent.error());
    }

    RenderFramePlan plan{
        .snapshot = std::move(snapshot),
        .opaque_queue = std::move(*opaque),
        .transparent_queue = std::move(*transparent),
    };
    auto graph = features.Build({
        .view = view,
        .snapshot = plan.snapshot,
        .opaque_queue = plan.opaque_queue,
        .transparent_queue = plan.transparent_queue,
    });
    if (!graph) {
        return Err(graph.error());
    }
    plan.feature_graph = std::move(*graph);
    return Ok(std::move(plan));
}

RenderFramePlanner::RenderFramePlanner(RenderScene& scene, RenderFeatureRegistry& features) noexcept
    : scene_(&scene), features_(&features) {}

Result<RenderFramePlan> RenderFramePlanner::Prepare(const RenderView& view, const u64 layer_mask) {
    auto snapshot = scene_->Extract(layer_mask);
    if (!snapshot) {
        return Err(snapshot.error());
    }
    return BuildRenderFramePlan(std::move(*snapshot), *features_, view);
}

} // namespace woki::gfx
