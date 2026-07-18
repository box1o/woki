#include <catch2/catch_test_macros.hpp>

#include <limits>

#include <woki/gfx/features.hpp>

namespace {

[[nodiscard]] woki::gfx::CascadedShadowFeatureDesc ValidDesc() {
    return {
        .cascades = {{.far_distance = 10.0F}, {.far_distance = 30.0F},
            {.far_distance = 80.0F}, {.far_distance = 200.0F}},
    };
}

} // namespace

TEST_CASE("Cascaded shadows validate one to four increasing splits") {
    REQUIRE(woki::gfx::Validate(ValidDesc()));

    auto desc = ValidDesc();
    desc.cascades.clear();
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc = ValidDesc();
    desc.cascades.push_back({.far_distance = 400.0F});
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc = ValidDesc();
    desc.cascades[2].far_distance = desc.cascades[1].far_distance;
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}

TEST_CASE("Cascaded shadows reject invalid atlas and bias settings") {
    auto desc = ValidDesc();
    desc.cascade_resolution = 0;
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc = ValidDesc();
    desc.strength = 1.1F;
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc = ValidDesc();
    desc.cascades.front().far_distance =
        std::numeric_limits<woki::f32>::infinity();
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}

TEST_CASE("Shadow frame data preserves the four-cascade GPU layout") {
    woki::gfx::ShadowFrameData data{};
    REQUIRE(data.cascade_count == 1);
    REQUIRE(data.atlas_transforms[0] == woki::math::vec4f{1.0F, 1.0F, 0.0F, 0.0F});
    REQUIRE(sizeof(data) == 368);
}
