#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/materials.hpp>

TEST_CASE("Material parameters preserve strong parameter types") {
    woki::gfx::MaterialParameters parameters;
    const woki::StringId roughness{"roughness"};

    parameters.Set(roughness, 0.35F);

    REQUIRE(parameters.Contains(roughness));
    REQUIRE(parameters.TryGet<woki::f32>(roughness) != nullptr);
    REQUIRE(*parameters.TryGet<woki::f32>(roughness) == 0.35F);
    REQUIRE(parameters.TryGet<woki::u32>(roughness) == nullptr);
    REQUIRE(parameters.Remove(roughness));
    REQUIRE(parameters.Empty());
}

TEST_CASE("PBR material factory produces standard material parameters") {
    woki::gfx::PbrMaterialDesc pbr{};
    pbr.asset_id = woki::gfx::AssetId{"materials/painted_metal"};
    pbr.label = "Painted metal";
    pbr.metallic = 0.8F;
    pbr.roughness = 0.2F;
    pbr.base_color_texture = woki::gfx::TextureHandle::FromParts(5, 1);

    const auto shader = woki::gfx::ShaderHandle::FromParts(3, 1);
    const auto material = woki::gfx::MakePbrMaterial(pbr, shader);

    REQUIRE(material.model == woki::gfx::MaterialModel::PhysicallyBased);
    REQUIRE(material.shader == shader);
    REQUIRE(material.depth_write);
    REQUIRE(
        *material.parameters.TryGet<woki::f32>(woki::gfx::material_parameters::kMetallic) == 0.8F);
    REQUIRE(*material.parameters.TryGet<woki::gfx::TextureHandle>(
                woki::gfx::material_parameters::kBaseColorTexture) == pbr.base_color_texture);
}

TEST_CASE("Translucent PBR materials disable depth writes by default") {
    woki::gfx::PbrMaterialDesc pbr{};
    pbr.blend_mode = woki::gfx::MaterialBlendMode::Translucent;

    const auto material = woki::gfx::MakePbrMaterial(pbr);

    REQUIRE(material.blend_mode == woki::gfx::MaterialBlendMode::Translucent);
    REQUIRE_FALSE(material.depth_write);
}

TEST_CASE("Material descriptions require shaders and valid physical ranges") {
    woki::gfx::MaterialDesc material{};
    REQUIRE_FALSE(woki::gfx::Validate(material).has_value());

    material.shader = woki::gfx::ShaderHandle::FromParts(1, 1);
    material.parameters.Set(woki::gfx::material_parameters::kRoughness, 0.5F);
    REQUIRE(woki::gfx::Validate(material).has_value());

    material.parameters.Set(woki::gfx::material_parameters::kRoughness, 1.5F);
    REQUIRE_FALSE(woki::gfx::Validate(material).has_value());
}

TEST_CASE("Masked materials require an alpha cutoff") {
    woki::gfx::MaterialDesc material{};
    material.shader = woki::gfx::ShaderHandle::FromParts(1, 1);
    material.blend_mode = woki::gfx::MaterialBlendMode::Masked;
    REQUIRE_FALSE(woki::gfx::Validate(material).has_value());

    material.parameters.Set(woki::gfx::material_parameters::kAlphaCutoff, 0.5F);
    REQUIRE(woki::gfx::Validate(material).has_value());
}
