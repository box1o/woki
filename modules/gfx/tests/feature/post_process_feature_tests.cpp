#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/features.hpp>

TEST_CASE("Post-process feature validates bindings") {
    woki::gfx::PostProcessFeatureDesc desc{
        .pipeline = woki::gfx::PipelineHandle::FromParts(0, 1),
        .sampler = woki::gfx::SamplerHandle::FromParts(0, 1),
    };
    REQUIRE(woki::gfx::Validate(desc));

    desc.sampler_binding = desc.texture_binding;
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}
