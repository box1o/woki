#include <woki/gfx/lighting/light.hpp>

#include <cmath>
#include <cstring>
#include <numbers>

namespace woki::gfx {
namespace {

[[nodiscard]] bool IsFinite(const math::vec3f value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace

Result<void> Validate(const LightDesc& desc) {
    if (!std::isfinite(desc.intensity) || desc.intensity < 0.0F) {
        return Err(
            ErrorCode::ValidationOutOfRange, "Light intensity must be finite and nonnegative");
    }
    if (!IsFinite(desc.color) || desc.color.x < 0.0F || desc.color.y < 0.0F ||
        desc.color.z < 0.0F) {
        return Err(ErrorCode::ValidationOutOfRange, "Light color cannot be negative");
    }
    if (!IsFinite(desc.position) || !IsFinite(desc.direction)) {
        return Err(ErrorCode::ValidationOutOfRange, "Light position and direction must be finite");
    }
    if (desc.type != LightType::Directional && (!std::isfinite(desc.range) || desc.range <= 0.0F)) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Point and spot lights require a positive finite range");
    }
    if (desc.type != LightType::Point && desc.direction.length_squared() <= 0.0F) {
        return Err(
            ErrorCode::ValidationNullValue, "Directional and spot lights require a direction");
    }
    if (desc.type == LightType::Spot &&
        (!std::isfinite(desc.inner_cone) || !std::isfinite(desc.outer_cone) ||
            desc.inner_cone < 0.0F || desc.outer_cone < desc.inner_cone ||
            desc.outer_cone >= std::numbers::pi_v<f32> * 0.5F)) {
        return Err(ErrorCode::ValidationOutOfRange, "Spot light cone angles are invalid");
    }
    return Ok();
}

Result<PackedLighting> PackLighting(const std::span<const ExtractedLight> lights,
    const math::vec3f ambient, const u32 maximum_lights) {
    if (maximum_lights == 0 || lights.size() > maximum_lights) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Extracted lighting exceeds the configured GPU light capacity");
    }
    const u64 byte_size = sizeof(GpuLightingHeader) + sizeof(GpuLight) * lights.size();
    PackedLighting packed{
        .bytes = std::vector<std::byte>(static_cast<std::size_t>(byte_size)),
        .light_count = static_cast<u32>(lights.size()),
    };
    const GpuLightingHeader header{
        .ambient = math::vec4f{ambient.x, ambient.y, ambient.z, 1.0F},
        .light_count = packed.light_count,
    };
    std::memcpy(packed.bytes.data(), &header, sizeof(header));
    for (std::size_t index = 0; index < lights.size(); ++index) {
        const auto& light = lights[index];
        const GpuLight output{
            .position_range = {light.position.x, light.position.y, light.position.z, light.range},
            .direction_type = {light.direction.x, light.direction.y, light.direction.z,
                static_cast<f32>(light.type)},
            .color_intensity = {light.color.x, light.color.y, light.color.z, light.intensity},
            .spot_shadow = {light.inner_cone, light.outer_cone, light.casts_shadows ? 1.0F : 0.0F,
                0.0F},
        };
        std::memcpy(packed.bytes.data() + sizeof(header) + index * sizeof(GpuLight), &output,
            sizeof(output));
    }
    return Ok(std::move(packed));
}

} // namespace woki::gfx
