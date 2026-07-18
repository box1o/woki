#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/features.hpp>

TEST_CASE("Depth prepass validates its target and clear value") {
    woki::gfx::DepthPrepassFeatureDesc desc{};
    REQUIRE(woki::gfx::Validate(desc));

    desc.format = woki::rhi::TextureFormat::Undefined;
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc.format = woki::rhi::TextureFormat::Depth32Float;
    desc.clear_depth = 1.1F;
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc.clear_depth = 1.0F;
    desc.sample_count = 0;
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}
