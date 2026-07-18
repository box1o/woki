#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/pipelines.hpp>

namespace {

[[nodiscard]] woki::gfx::StandardMaterialPipelineDesc MakeDesc() {
    return {
        .asset_id = woki::gfx::AssetId{"pipelines/pbr_opaque"},
        .label = "PBR opaque",
        .shader = woki::gfx::ShaderHandle::FromParts(0, 1),
        .targets = {.color_formats = {woki::rhi::TextureFormat::RGBA16Float},
            .depth_format = woki::rhi::TextureFormat::Depth32Float},
        .vertex_layout = woki::StringId{"position_normal"},
        .vertex_buffers = {{.stride = 24,
            .attributes =
                {
                    {.format = woki::rhi::VertexFormat::Float32x3,
                        .offset = 0,
                        .shader_location = 0},
                    {.format = woki::rhi::VertexFormat::Float32x3,
                        .offset = 12,
                        .shader_location = 1},
                }}},
    };
}

} // namespace

TEST_CASE("Standard material pipeline builds opaque forward state") {
    const auto built = woki::gfx::BuildStandardMaterialPipeline(MakeDesc());
    REQUIRE(built);
    REQUIRE(built->graphics.color_targets.size() == 1);
    REQUIRE_FALSE(built->graphics.color_targets.front().blend);
    REQUIRE(built->graphics.depth_stencil);
    REQUIRE(built->graphics.depth_stencil->depth_write_enabled == true);
    REQUIRE(built->key.pass == woki::gfx::RenderPassClass::ForwardOpaque);
    REQUIRE(built->graphics.sample_count == 1);
}

TEST_CASE("Standard material pipeline carries multisample state") {
    auto desc = MakeDesc();
    desc.asset_id = woki::gfx::AssetId{"pipelines/pbr_opaque_msaa"};
    desc.targets.sample_count = 4;
    const auto built = woki::gfx::BuildStandardMaterialPipeline(desc);
    REQUIRE(built);
    REQUIRE(built->graphics.sample_count == 4);
    REQUIRE(built->key.targets.sample_count == 4);
}

TEST_CASE("Standard material pipeline builds transparent blending") {
    auto desc = MakeDesc();
    desc.asset_id = woki::gfx::AssetId{"pipelines/pbr_transparent"};
    desc.blend_mode = woki::gfx::MaterialBlendMode::Translucent;
    desc.pass = woki::gfx::RenderPassClass::ForwardTransparent;
    const auto built = woki::gfx::BuildStandardMaterialPipeline(desc);
    REQUIRE(built);
    REQUIRE(built->graphics.color_targets.front().blend);
    REQUIRE(built->graphics.depth_stencil->depth_write_enabled == false);
    REQUIRE_FALSE(built->key.depth_write);
}

TEST_CASE("Standard material pipeline builds depth-only variants") {
    auto desc = MakeDesc();
    desc.asset_id = woki::gfx::AssetId{"pipelines/pbr_depth"};
    desc.pass = woki::gfx::RenderPassClass::DepthOnly;
    desc.targets.color_formats.clear();
    const auto built = woki::gfx::BuildStandardMaterialPipeline(desc);
    REQUIRE(built);
    REQUIRE(built->graphics.color_targets.empty());
    REQUIRE(built->graphics.depth_stencil);
}

TEST_CASE("Masked depth pipelines require and use a fragment implementation") {
    auto desc = MakeDesc();
    desc.asset_id = woki::gfx::AssetId{"pipelines/pbr_masked_depth"};
    desc.blend_mode = woki::gfx::MaterialBlendMode::Masked;
    desc.pass = woki::gfx::RenderPassClass::DepthOnly;
    desc.targets.color_formats.clear();
    REQUIRE_FALSE(woki::gfx::BuildStandardMaterialPipeline(desc));

    desc.implementation_shader = woki::gfx::ShaderHandle::FromParts(1, 1);
    desc.depth_fragment = true;
    const auto built = woki::gfx::BuildStandardMaterialPipeline(desc);
    REQUIRE(built);
    REQUIRE(built->key.shader == desc.shader);
    REQUIRE(built->graphics.shader == desc.implementation_shader);
    REQUIRE(built->graphics.depth_fragment);
}
