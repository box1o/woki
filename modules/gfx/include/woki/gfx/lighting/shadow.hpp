#pragma once

#include <woki/core.hpp>
#include <woki/math/math.hpp>

#include <array>
#include <cstddef>

namespace woki::gfx {

namespace render_outputs {
inline const StringId kShadowDepth{"render.shadow_depth"};
inline const StringId kShadowData{"render.shadow_data"};
} // namespace render_outputs

struct alignas(16) ShadowFrameData final {
    static constexpr u32 kMaximumCascades = 4;

    std::array<math::mat4f, kMaximumCascades> light_view_projections{
        math::mat4f::identity(), math::mat4f::identity(), math::mat4f::identity(),
        math::mat4f::identity()};
    std::array<math::vec4f, kMaximumCascades> atlas_transforms{
        math::vec4f{1.0F, 1.0F, 0.0F, 0.0F}, math::vec4f{}, math::vec4f{}, math::vec4f{}};
    math::vec4f split_distances{};
    math::vec4f parameters{0.001F, 0.002F, 1.0F, 0.0F};
    u32 light_index{0};
    u32 cascade_count{1};
    u32 padding[2]{};
};

static_assert(sizeof(ShadowFrameData) == 368);
static_assert(offsetof(ShadowFrameData, atlas_transforms) == 256);
static_assert(offsetof(ShadowFrameData, split_distances) == 320);
static_assert(offsetof(ShadowFrameData, parameters) == 336);
static_assert(offsetof(ShadowFrameData, light_index) == 352);
static_assert(offsetof(ShadowFrameData, cascade_count) == 356);

} // namespace woki::gfx
