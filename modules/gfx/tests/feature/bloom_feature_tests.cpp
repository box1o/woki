#include <catch2/catch_test_macros.hpp>

#include <limits>

#include <woki/gfx/features.hpp>

namespace {

[[nodiscard]] woki::gfx::BloomFeatureDesc ValidDesc() {
    return {
        .threshold_pipeline = woki::gfx::PipelineHandle::FromParts(0, 1),
        .blur_pipeline = woki::gfx::PipelineHandle::FromParts(1, 1),
        .composite_pipeline = woki::gfx::PipelineHandle::FromParts(2, 1),
        .sampler = woki::gfx::SamplerHandle::FromParts(0, 1),
    };
}

} // namespace

TEST_CASE("Bloom feature validates a production configuration") {
    REQUIRE(woki::gfx::Validate(ValidDesc()));
}

TEST_CASE("Bloom feature rejects missing resources") {
    auto desc = ValidDesc();
    desc.blur_pipeline = {};
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc = ValidDesc();
    desc.sampler = {};
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}

TEST_CASE("Bloom feature validates quality and light extraction settings") {
    auto desc = ValidDesc();
    desc.resolution_scale = 0.0F;
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc = ValidDesc();
    desc.blur_iterations = 0;
    REQUIRE_FALSE(woki::gfx::Validate(desc));
    desc.blur_iterations = 9;
    REQUIRE_FALSE(woki::gfx::Validate(desc));

    desc = ValidDesc();
    desc.threshold = -1.0F;
    REQUIRE_FALSE(woki::gfx::Validate(desc));
    desc.threshold = std::numeric_limits<woki::f32>::infinity();
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}
