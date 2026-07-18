#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/pipelines.hpp>

TEST_CASE("Graphics pipeline descriptions require shader and attachments") {
    woki::gfx::GraphicsPipelineDesc desc{};
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());

    desc.shader = woki::gfx::ShaderHandle::FromParts(1, 1);
    desc.color_targets.push_back({.format = woki::rhi::TextureFormat::BGRA8Unorm});
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.sample_count = 0;
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}

TEST_CASE("Graphics pipeline vertex layouts require valid unique attributes") {
    woki::gfx::GraphicsPipelineDesc desc{};
    desc.shader = woki::gfx::ShaderHandle::FromParts(1, 1);
    desc.color_targets.push_back({.format = woki::rhi::TextureFormat::BGRA8Unorm});
    desc.vertex_buffers.push_back({
        .stride = 24,
        .attributes =
            {
                {.format = woki::rhi::VertexFormat::Float32x3, .offset = 0, .shader_location = 0},
                {.format = woki::rhi::VertexFormat::Float32x3, .offset = 12, .shader_location = 1},
            },
    });
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.vertex_buffers.front().attributes.back().shader_location = 0;
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Depth-only graphics pipelines are valid") {
    woki::gfx::GraphicsPipelineDesc desc{};
    desc.shader = woki::gfx::ShaderHandle::FromParts(2, 1);
    desc.depth_stencil = woki::rhi::DepthStencilStateDesc{
        .format = woki::rhi::TextureFormat::Depth32Float,
    };

    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.depth_fragment = true;
    REQUIRE(woki::gfx::Validate(desc));
    desc.color_targets.push_back({.format = woki::rhi::TextureFormat::RGBA8Unorm});
    REQUIRE_FALSE(woki::gfx::Validate(desc));
}
