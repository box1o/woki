#include <catch2/catch_test_macros.hpp>

#include <cstring>

#include <woki/gfx/lighting.hpp>

TEST_CASE("Renderer lights validate type-specific properties") {
    woki::gfx::LightDesc directional{};
    REQUIRE(woki::gfx::Validate(directional));

    directional.direction = {};
    REQUIRE_FALSE(woki::gfx::Validate(directional));

    woki::gfx::LightDesc point{.type = woki::gfx::LightType::Point, .range = 0.0F};
    REQUIRE_FALSE(woki::gfx::Validate(point));

    woki::gfx::LightDesc spot{
        .type = woki::gfx::LightType::Spot, .inner_cone = 0.8F, .outer_cone = 0.4F};
    REQUIRE_FALSE(woki::gfx::Validate(spot));
}

TEST_CASE("Extracted lights pack into a stable GPU layout") {
    const std::vector<woki::gfx::ExtractedLight> lights{{
        .light = woki::gfx::LightHandle::FromParts(1, 1),
        .type = woki::gfx::LightType::Point,
        .color = {0.5F, 0.75F, 1.0F},
        .intensity = 4.0F,
        .position = {1.0F, 2.0F, 3.0F},
        .range = 12.0F,
        .casts_shadows = true,
    }};

    auto packed = woki::gfx::PackLighting(lights, {0.1F, 0.2F, 0.3F});

    REQUIRE(packed);
    REQUIRE(packed->light_count == 1);
    REQUIRE(
        packed->bytes.size() == sizeof(woki::gfx::GpuLightingHeader) + sizeof(woki::gfx::GpuLight));
    woki::gfx::GpuLightingHeader header{};
    woki::gfx::GpuLight light{};
    std::memcpy(&header, packed->bytes.data(), sizeof(header));
    std::memcpy(&light, packed->bytes.data() + sizeof(header), sizeof(light));
    REQUIRE(header.light_count == 1);
    REQUIRE(header.ambient.x == 0.1F);
    REQUIRE(light.position_range.w == 12.0F);
    REQUIRE(light.color_intensity.w == 4.0F);
    REQUIRE(light.spot_shadow.z == 1.0F);
}

TEST_CASE("Lighting packing enforces the configured capacity") {
    const std::vector<woki::gfx::ExtractedLight> lights(2);
    REQUIRE_FALSE(woki::gfx::PackLighting(lights, {}, 1));
}
