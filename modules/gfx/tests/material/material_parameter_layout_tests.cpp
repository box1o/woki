#include <catch2/catch_test_macros.hpp>

#include <cstring>

#include <woki/gfx/materials.hpp>

TEST_CASE("Material parameters use uniform-compatible alignment") {
    const std::vector<woki::gfx::ShaderParameterDesc> parameters{
        {.name = woki::StringId{"scalar"}, .type = woki::gfx::ShaderValueType::Float32},
        {.name = woki::StringId{"direction"}, .type = woki::gfx::ShaderValueType::Float32x3},
        {.name = woki::StringId{"factor"}, .type = woki::gfx::ShaderValueType::Float32},
    };

    auto layout = woki::gfx::BuildMaterialParameterLayout(parameters);

    REQUIRE(layout);
    REQUIRE(layout->fields[0].offset == 0);
    REQUIRE(layout->fields[1].offset == 16);
    REQUIRE(layout->fields[2].offset == 28);
    REQUIRE(layout->byte_size == 32);
}

TEST_CASE("Material parameters pack typed values deterministically") {
    const woki::StringId enabled{"enabled"};
    const woki::StringId roughness{"roughness"};
    const std::vector<woki::gfx::ShaderParameterDesc> descriptions{
        {.name = enabled, .type = woki::gfx::ShaderValueType::Bool, .required = true},
        {.name = roughness, .type = woki::gfx::ShaderValueType::Float32, .required = true},
    };
    auto layout = woki::gfx::BuildMaterialParameterLayout(descriptions);
    REQUIRE(layout);
    woki::gfx::MaterialParameters parameters{};
    parameters.Set(enabled, true);
    parameters.Set(roughness, 0.35F);

    auto packed = woki::gfx::PackMaterialParameters(*layout, parameters);

    REQUIRE(packed);
    woki::u32 encoded_enabled = 0;
    woki::f32 encoded_roughness = 0.0F;
    std::memcpy(&encoded_enabled, packed->bytes.data(), sizeof(encoded_enabled));
    std::memcpy(&encoded_roughness, packed->bytes.data() + 4, sizeof(encoded_roughness));
    REQUIRE(encoded_enabled == 1);
    REQUIRE(encoded_roughness == 0.35F);
}

TEST_CASE("Material parameter packing rejects missing and mismatched values") {
    const woki::StringId value{"value"};
    const std::vector<woki::gfx::ShaderParameterDesc> descriptions{
        {.name = value, .type = woki::gfx::ShaderValueType::Float32, .required = true}};
    auto layout = woki::gfx::BuildMaterialParameterLayout(descriptions);
    REQUIRE(layout);
    woki::gfx::MaterialParameters parameters{};
    REQUIRE_FALSE(woki::gfx::PackMaterialParameters(*layout, parameters));

    parameters.Set(value, woki::u32{4});
    REQUIRE_FALSE(woki::gfx::PackMaterialParameters(*layout, parameters));
}

TEST_CASE("Material resource bindings validate logical handle types") {
    const woki::StringId texture_name{"albedo"};
    woki::gfx::ShaderInterfaceDesc interface{
        .resources = {{.name = texture_name,
            .type = woki::gfx::ShaderResourceType::Texture2D,
            .required = true}},
    };
    woki::gfx::MaterialParameters parameters{};
    parameters.Set(texture_name, woki::gfx::SamplerHandle::FromParts(1, 1));
    REQUIRE_FALSE(woki::gfx::ValidateMaterialResources(interface, parameters));

    parameters.Set(texture_name, woki::gfx::TextureHandle::FromParts(1, 1));
    REQUIRE(woki::gfx::ValidateMaterialResources(interface, parameters));
}
