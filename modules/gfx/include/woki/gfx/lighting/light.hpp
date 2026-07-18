#pragma once

#include "../resource/resource_types.hpp"

#include <cstddef>
#include <span>
#include <vector>

#include <woki/math/math.hpp>

namespace woki::gfx {

enum class LightType : u8 {
    Directional = 0,
    Point,
    Spot,
};

struct LightDesc final {
    LightType type{LightType::Directional};
    math::vec3f color{1.0F};
    f32 intensity{1.0F};
    math::vec3f position{};
    f32 range{10.0F};
    math::vec3f direction{0.0F, -1.0F, 0.0F};
    f32 inner_cone{0.35F};
    f32 outer_cone{0.5F};
    u64 layer_mask{~0ULL};
    bool enabled{true};
    bool casts_shadows{false};
};

struct ExtractedLight final {
    LightHandle light{};
    LightType type{LightType::Directional};
    math::vec3f color{1.0F};
    f32 intensity{1.0F};
    math::vec3f position{};
    f32 range{0.0F};
    math::vec3f direction{0.0F, -1.0F, 0.0F};
    f32 inner_cone{0.0F};
    f32 outer_cone{0.0F};
    bool casts_shadows{false};
};

struct alignas(16) GpuLightingHeader final {
    math::vec4f ambient{};
    u32 light_count{0};
    u32 padding[3]{};
};

struct alignas(16) GpuLight final {
    math::vec4f position_range{};
    math::vec4f direction_type{};
    math::vec4f color_intensity{};
    math::vec4f spot_shadow{};
};

struct PackedLighting final {
    std::vector<std::byte> bytes{};
    u32 light_count{0};
};

[[nodiscard]] Result<void> Validate(const LightDesc& desc);
[[nodiscard]] Result<PackedLighting> PackLighting(
    std::span<const ExtractedLight> lights, math::vec3f ambient = {}, u32 maximum_lights = 256);

} // namespace woki::gfx
