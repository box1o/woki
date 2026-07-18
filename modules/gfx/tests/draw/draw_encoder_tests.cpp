#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/draws.hpp>

TEST_CASE("Draw encoder validation accepts an empty draw list") {
    REQUIRE(woki::gfx::Validate(woki::gfx::ResolvedDrawList{}));
}

TEST_CASE("Draw encoder validation rejects unresolved native state") {
    woki::gfx::ResolvedDrawList draws{
        .draws = {{.packet = {.index_count = 3}}},
        .batches = {{.draw_count = 1}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(draws));
}

TEST_CASE("Draw encoder validation requires complete contiguous batches") {
    auto* pipeline = reinterpret_cast<const woki::rhi::RenderPipeline*>(1);
    auto* buffer = reinterpret_cast<const woki::rhi::Buffer*>(1);
    woki::gfx::ResolvedDrawList draws{
        .draws = {{
            .packet = {.index_count = 3},
            .pipeline = pipeline,
            .vertex_buffers = {buffer},
            .index_buffer = buffer,
            .index_format = woki::rhi::IndexFormat::Uint32,
        }},
        .batches = {{.pipeline = pipeline, .first_draw = 1, .draw_count = 1}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(draws));
}
