#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/pipelines.hpp>

namespace {

const woki::StringId kVertexLayout{"static_mesh"};

[[nodiscard]] woki::gfx::MaterialDesc OpaqueMaterial() {
    return {
        .model = woki::gfx::MaterialModel::PhysicallyBased,
        .shader = woki::gfx::ShaderHandle::FromParts(1, 1),
    };
}

[[nodiscard]] woki::gfx::RenderTargetSignature HdrTargets() {
    return {
        .color_formats = {woki::rhi::TextureFormat::RGBA16Float},
        .depth_format = woki::rhi::TextureFormat::Depth24Plus,
    };
}

} // namespace

TEST_CASE("Material pipeline resolver selects an exact render variant") {
    woki::gfx::MaterialPipelineResolver resolver{};
    const auto material = OpaqueMaterial();
    const auto targets = HdrTargets();
    const auto key = woki::gfx::MakeMaterialPipelineKey(
        material, woki::gfx::RenderPassClass::ForwardOpaque, targets, kVertexLayout);
    const auto pipeline = woki::gfx::PipelineHandle::FromParts(3, 1);

    REQUIRE(resolver.Register(key, pipeline));

    REQUIRE(resolver.Resolve(material, woki::gfx::RenderPassClass::ForwardOpaque, targets,
                kVertexLayout) == pipeline);
    REQUIRE(resolver.Size() == 1);
}

TEST_CASE("Material pipeline variants include attachment and material state") {
    woki::gfx::MaterialPipelineResolver resolver{};
    auto material = OpaqueMaterial();
    const auto targets = HdrTargets();
    const auto key = woki::gfx::MakeMaterialPipelineKey(
        material, woki::gfx::RenderPassClass::ForwardOpaque, targets, kVertexLayout);
    REQUIRE(resolver.Register(key, woki::gfx::PipelineHandle::FromParts(1, 1)));

    material.double_sided = true;
    REQUIRE_FALSE(resolver.Resolve(
        material, woki::gfx::RenderPassClass::ForwardOpaque, targets, kVertexLayout));

    auto other_targets = targets;
    other_targets.sample_count = 4;
    REQUIRE_FALSE(resolver.Resolve(
        OpaqueMaterial(), woki::gfx::RenderPassClass::ForwardOpaque, other_targets, kVertexLayout));
}

TEST_CASE("Material pipeline resolver rejects pass and blend mismatches") {
    auto material = OpaqueMaterial();
    const auto key = woki::gfx::MakeMaterialPipelineKey(
        material, woki::gfx::RenderPassClass::ForwardTransparent, HdrTargets(), kVertexLayout);
    woki::gfx::MaterialPipelineResolver resolver{};

    REQUIRE_FALSE(resolver.Register(key, woki::gfx::PipelineHandle::FromParts(1, 1)));
}

TEST_CASE("Material pipeline variants can be atomically replaced") {
    woki::gfx::MaterialPipelineResolver resolver{};
    const auto key = woki::gfx::MakeMaterialPipelineKey(
        OpaqueMaterial(), woki::gfx::RenderPassClass::ForwardOpaque, HdrTargets(), kVertexLayout);
    const auto first = woki::gfx::PipelineHandle::FromParts(1, 1);
    const auto replacement = woki::gfx::PipelineHandle::FromParts(2, 1);
    REQUIRE(resolver.Register(key, first));

    REQUIRE(resolver.Replace(key, replacement));

    REQUIRE(resolver.Resolve(key) == replacement);
    REQUIRE(resolver.RemovePipeline(replacement) == 1);
    REQUIRE_FALSE(resolver.Resolve(key));
}
