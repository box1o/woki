#pragma once

#include "../resource/resource_types.hpp"

#include <woki/core.hpp>
#include <woki/math/math.hpp>

namespace woki::gfx {

struct EnvironmentLighting final {
    TextureHandle radiance{};
    TextureHandle irradiance{};
    TextureHandle brdf_lut{};
    SamplerHandle sampler{};
    f32 intensity{1.0F};
    f32 maximum_reflection_lod{0.0F};
    f32 rotation{0.0F};
};

struct alignas(16) GpuEnvironmentData final {
    math::vec4f parameters{};
};

[[nodiscard]] Result<void> Validate(const EnvironmentLighting& environment);
[[nodiscard]] GpuEnvironmentData PackEnvironment(const EnvironmentLighting& environment) noexcept;

} // namespace woki::gfx
