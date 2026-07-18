#pragma once

#include "render_scene.hpp"

#include <optional>
#include <vector>

namespace woki::gfx {

struct RenderQueueFilter final {
    std::optional<DrawPhase> phase{};
    u64 layer_mask{~0ULL};
    bool shadow_casters_only{false};
    std::optional<Frustum> frustum{};
};

struct QueuedDraw final {
    DrawPacket packet{};
    math::mat4f transform{math::mat4f::identity()};
    std::vector<math::mat4f> skin_matrices{};
};

struct DrawBatch final {
    MeshHandle mesh{};
    MaterialHandle material{};
    DrawPhase phase{DrawPhase::Opaque};
    u32 first_draw{0};
    u32 draw_count{0};
};

struct RenderQueue final {
    u64 snapshot_sequence{0};
    std::vector<QueuedDraw> draws{};
    std::vector<DrawBatch> batches{};
};

[[nodiscard]] Result<RenderQueue> BuildRenderQueue(
    const RenderSnapshot& snapshot, const RenderQueueFilter& filter = {});

} // namespace woki::gfx
