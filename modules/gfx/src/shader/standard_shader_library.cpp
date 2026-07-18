#include <woki/gfx/shader/standard_shader_library.hpp>

#include <woki/gfx/material/material.hpp>
#include <woki/gfx/shader/shader_manager.hpp>

#include <utility>

namespace woki::gfx {
namespace {

[[nodiscard]] ShaderInterfaceDesc StandardInterface(
    const bool lighting, const bool skinning, const bool shadows) {
    ShaderInterfaceDesc interface{
        .uses_object_transform = true,
        .object_group = 0,
        .object_binding = 0,
        .uses_skinning = skinning,
        .parameter_group = 1,
        .parameter_binding = 0,
        .uses_lighting = lighting,
        .lighting_group = 2,
        .lighting_binding = 0,
        .uses_shadows = shadows,
        .shadow_group = 2,
    };
    if (lighting) {
        interface.parameters = {
            {.name = StringId{"base_color"}, .type = ShaderValueType::Float32x4},
            {.name = StringId{"emissive"}, .type = ShaderValueType::Float32x3},
            {.name = StringId{"metallic"}, .type = ShaderValueType::Float32},
            {.name = StringId{"roughness"}, .type = ShaderValueType::Float32},
        };
    } else {
        interface.parameters = {
            {.name = StringId{"color"}, .type = ShaderValueType::Float32x4},
        };
    }
    return interface;
}

void AddTexturedPbrInterface(ShaderInterfaceDesc& interface) {
    interface.parameters.push_back(
        {.name = material_parameters::kNormalScale, .type = ShaderValueType::Float32});
    interface.parameters.push_back(
        {.name = material_parameters::kOcclusionStrength, .type = ShaderValueType::Float32});
    interface.parameters.push_back(
        {.name = material_parameters::kAlphaCutoff, .type = ShaderValueType::Float32});
    interface.resources = {
        {.name = material_parameters::kBaseColorTexture,
            .type = ShaderResourceType::Texture2D,
            .group = 1,
            .binding = 1},
        {.name = material_parameters::kMetallicRoughnessTexture,
            .type = ShaderResourceType::Texture2D,
            .group = 1,
            .binding = 2},
        {.name = material_parameters::kNormalTexture,
            .type = ShaderResourceType::Texture2D,
            .group = 1,
            .binding = 3},
        {.name = material_parameters::kOcclusionTexture,
            .type = ShaderResourceType::Texture2D,
            .group = 1,
            .binding = 4},
        {.name = material_parameters::kEmissiveTexture,
            .type = ShaderResourceType::Texture2D,
            .group = 1,
            .binding = 5},
        {.name = material_parameters::kSampler,
            .type = ShaderResourceType::Sampler,
            .group = 1,
            .binding = 6},
    };
}

} // namespace

StandardShaderLibrary::StandardShaderLibrary(paths::Path shader_root)
    : root_(std::move(shader_root)) {}

const paths::Path& StandardShaderLibrary::Root() const noexcept { return root_; }

ShaderDesc StandardShaderLibrary::Describe(const StandardShader shader) const {
    if (shader == StandardShader::ToneMap) {
        const std::string source_path = (root_ / "tone_map.wgsl").generic_string();
        return {
            .asset_id = AssetId{"woki/shaders/tone_map"},
            .label = "Woki Tone Map",
            .sources =
                {
                    {.stage = ShaderStage::Vertex,
                        .entry_point = "vertex_main",
                        .source_path = source_path},
                    {.stage = ShaderStage::Fragment,
                        .entry_point = "fragment_main",
                        .source_path = source_path},
                },
            .interface = {.resources =
                              {
                                  {.name = StringId{"hdr_color"},
                                      .type = ShaderResourceType::Texture2D,
                                      .group = 0,
                                      .binding = 0,
                                      .required = true},
                                  {.name = StringId{"linear_sampler"},
                                      .type = ShaderResourceType::Sampler,
                                      .group = 0,
                                      .binding = 1,
                                      .required = true},
                              }},
            .hot_reload = true,
        };
    }
    const bool pbr = shader != StandardShader::Unlit;
    const bool skinned = shader == StandardShader::PbrSkinned;
    const bool textured = shader == StandardShader::PbrTextured;
    const bool shadowed = shader == StandardShader::PbrShadowed;
    const std::string name =
        skinned
            ? "pbr_skinned_forward"
            : (textured ? "pbr_textured_forward"
                        : (shadowed ? "pbr_shadowed_forward" : (pbr ? "pbr_forward" : "unlit")));
    const std::string source_path = (root_ / (name + ".wgsl")).generic_string();
    ShaderDesc result{
        .asset_id = AssetId{skinned    ? "woki/shaders/pbr_skinned"
                            : textured ? "woki/shaders/pbr_textured"
                            : shadowed ? "woki/shaders/pbr_shadowed"
                                       : (pbr ? "woki/shaders/pbr" : "woki/shaders/unlit")},
        .label = skinned    ? "Woki PBR Skinned"
                 : textured ? "Woki PBR Textured"
                 : shadowed ? "Woki PBR Shadowed"
                            : (pbr ? "Woki PBR" : "Woki Unlit"),
        .sources =
            {
                {.stage = ShaderStage::Vertex,
                    .entry_point = "vertex_main",
                    .source_path = source_path},
                {.stage = ShaderStage::Fragment,
                    .entry_point = "fragment_main",
                    .source_path = source_path},
            },
        .interface = StandardInterface(pbr, skinned, shadowed),
        .hot_reload = true,
    };
    if (textured) {
        AddTexturedPbrInterface(result.interface);
    }
    return result;
}

Result<ShaderHandle> StandardShaderLibrary::Load(
    const StandardShader shader, ShaderManager& manager) const {
    return manager.Create(Describe(shader));
}

} // namespace woki::gfx
