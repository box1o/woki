#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/lighting.hpp>

TEST_CASE("Environment lighting validates required resources") {
    woki::gfx::EnvironmentLighting environment{};
    REQUIRE_FALSE(woki::gfx::Validate(environment));

    environment.radiance = woki::gfx::TextureHandle::FromParts(0, 1);
    environment.irradiance = woki::gfx::TextureHandle::FromParts(1, 1);
    environment.brdf_lut = woki::gfx::TextureHandle::FromParts(2, 1);
    environment.sampler = woki::gfx::SamplerHandle::FromParts(0, 1);
    environment.maximum_reflection_lod = 8.0F;
    REQUIRE(woki::gfx::Validate(environment));
}

TEST_CASE("Environment lighting packs rotation and reflection range") {
    woki::gfx::EnvironmentLighting environment{
        .radiance = woki::gfx::TextureHandle::FromParts(0, 1),
        .irradiance = woki::gfx::TextureHandle::FromParts(1, 1),
        .brdf_lut = woki::gfx::TextureHandle::FromParts(2, 1),
        .sampler = woki::gfx::SamplerHandle::FromParts(0, 1),
        .intensity = 2.0F,
        .maximum_reflection_lod = 7.0F,
    };
    const auto packed = woki::gfx::PackEnvironment(environment);
    REQUIRE(packed.parameters.x == 2.0F);
    REQUIRE(packed.parameters.y == 7.0F);
    REQUIRE(packed.parameters.z == 0.0F);
    REQUIRE(packed.parameters.w == 1.0F);
}
