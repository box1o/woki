#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/features.hpp>

TEST_CASE("Shadow feature validates its render target") {
    woki::gfx::ShadowRenderFeatureDesc desc{};
    REQUIRE(woki::gfx::Validate(desc));

    desc.resolution = 0;
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc.resolution = 1024;
    desc.clear_depth = 2.0F;
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}
