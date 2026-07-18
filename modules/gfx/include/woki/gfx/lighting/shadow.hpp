#pragma once

#include <woki/core.hpp>
#include <woki/math/math.hpp>

namespace woki::gfx {

namespace render_outputs {
inline const StringId kShadowDepth{"render.shadow_depth"};
inline const StringId kShadowData{"render.shadow_data"};
} // namespace render_outputs

struct alignas(16) ShadowFrameData final {
    math::mat4f light_view_projection{math::mat4f::identity()};
    f32 depth_bias{0.001F};
    f32 normal_bias{0.002F};
    f32 strength{1.0F};
    u32 light_index{0};
};

} // namespace woki::gfx
