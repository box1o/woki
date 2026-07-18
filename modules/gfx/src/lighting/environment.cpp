#include <woki/gfx/lighting/environment.hpp>

#include <cmath>

namespace woki::gfx {

Result<void> Validate(const EnvironmentLighting& environment) {
    if (!environment.radiance || !environment.irradiance || !environment.brdf_lut ||
        !environment.sampler) {
        return Err(ErrorCode::ValidationNullValue,
            "Environment lighting requires radiance, irradiance, BRDF LUT, and sampler resources");
    }
    if (!std::isfinite(environment.intensity) || environment.intensity < 0.0F ||
        !std::isfinite(environment.maximum_reflection_lod) ||
        environment.maximum_reflection_lod < 0.0F || !std::isfinite(environment.rotation)) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Environment lighting parameters must be finite and nonnegative");
    }
    return Ok();
}

GpuEnvironmentData PackEnvironment(const EnvironmentLighting& environment) noexcept {
    return {.parameters = {environment.intensity, environment.maximum_reflection_lod,
                std::sin(environment.rotation), std::cos(environment.rotation)}};
}

} // namespace woki::gfx
