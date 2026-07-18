#include <woki/gfx/shader/standard_shader_library.hpp>

#include <woki/gfx/material/material.hpp>
#include <woki/gfx/shader/shader_manager.hpp>

#include <utility>

namespace woki::gfx {
namespace {

[[nodiscard]] ShaderInterfaceDesc StandardInterface(
    const bool lighting, const bool skinning, const bool shadows, const bool environment) {
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
        .uses_environment = environment,
        .environment_group = 2,
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

struct SurfaceShaderInfo final {
    std::string_view file{};
    std::string_view asset{};
    std::string_view label{};
    bool pbr{false};
    bool skinned{false};
    bool textured{false};
    bool shadows{false};
    bool environment{false};
};

[[nodiscard]] constexpr SurfaceShaderInfo SurfaceInfo(const StandardShader shader) noexcept {
    switch (shader) {
    case StandardShader::Pbr:
        return {"pbr_forward", "woki/shaders/pbr", "Woki PBR", true};
    case StandardShader::PbrShadowed:
        return {"pbr_shadowed_forward", "woki/shaders/pbr_shadowed", "Woki PBR Shadowed", true,
            false, false, true};
    case StandardShader::PbrEnvironment:
        return {"pbr_environment_forward", "woki/shaders/pbr_environment", "Woki PBR Environment",
            true, false, false, false, true};
    case StandardShader::PbrTextured:
        return {"pbr_textured_forward", "woki/shaders/pbr_textured", "Woki PBR Textured", true,
            false, true};
    case StandardShader::PbrSkinned:
        return {"pbr_skinned_forward", "woki/shaders/pbr_skinned", "Woki PBR Skinned", true, true};
    case StandardShader::PbrFull:
        return {"pbr_full_forward", "woki/shaders/pbr_full", "Woki PBR Full", true, false, true,
            true, true};
    case StandardShader::PbrFullSkinned:
        return {"pbr_full_skinned_forward", "woki/shaders/pbr_full_skinned",
            "Woki PBR Full Skinned", true, true, true, true, true};
    default:
        return {"unlit", "woki/shaders/unlit", "Woki Unlit"};
    }
}

} // namespace

StandardShaderLibrary::StandardShaderLibrary(paths::Path shader_root)
    : root_(std::move(shader_root)) {}

const paths::Path& StandardShaderLibrary::Root() const noexcept { return root_; }

ShaderDesc StandardShaderLibrary::Describe(const StandardShader shader) const {
    if (shader == StandardShader::BloomThreshold || shader == StandardShader::BloomBlur ||
        shader == StandardShader::BloomComposite) {
        const bool composite = shader == StandardShader::BloomComposite;
        const std::string_view file = shader == StandardShader::BloomThreshold
                                          ? "bloom_threshold"
                                      : shader == StandardShader::BloomBlur ? "bloom_blur"
                                                                           : "bloom_composite";
        const std::string_view asset = shader == StandardShader::BloomThreshold
                                           ? "woki/shaders/bloom_threshold"
                                       : shader == StandardShader::BloomBlur
                                           ? "woki/shaders/bloom_blur"
                                           : "woki/shaders/bloom_composite";
        const std::string_view label = shader == StandardShader::BloomThreshold
                                           ? "Woki Bloom Threshold"
                                       : shader == StandardShader::BloomBlur ? "Woki Bloom Blur"
                                                                            : "Woki Bloom Composite";
        const std::string source_path =
            (root_ / (std::string(file) + ".wgsl")).generic_string();
        ShaderInterfaceDesc interface{};
        interface.resources = {
            {.name = StringId{"source_color"},
                .type = ShaderResourceType::Texture2D,
                .group = 0,
                .binding = 0,
                .required = true},
        };
        if (composite) {
            interface.resources.push_back({.name = StringId{"bloom_color"},
                .type = ShaderResourceType::Texture2D,
                .group = 0,
                .binding = 1,
                .required = true});
        }
        interface.resources.push_back({.name = StringId{"linear_sampler"},
            .type = ShaderResourceType::Sampler,
            .group = 0,
            .binding = composite ? 2U : 1U,
            .required = true});
        return {
            .asset_id = AssetId{asset},
            .label = std::string(label),
            .sources =
                {
                    {.stage = ShaderStage::Vertex,
                        .entry_point = "vertex_main",
                        .source_path = source_path},
                    {.stage = ShaderStage::Fragment,
                        .entry_point = "fragment_main",
                        .source_path = source_path},
                },
            .interface = std::move(interface),
            .hot_reload = true,
        };
    }
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
    const bool masked_depth =
        shader == StandardShader::DepthMasked || shader == StandardShader::DepthMaskedSkinned;
    const bool skinned_depth =
        shader == StandardShader::DepthSkinned || shader == StandardShader::DepthMaskedSkinned;
    if (masked_depth || skinned_depth) {
        const std::string file = masked_depth
                                     ? (skinned_depth ? "depth_masked_skinned" : "depth_masked")
                                     : "depth_skinned";
        ShaderDesc result{
            .asset_id = AssetId{masked_depth ? (skinned_depth ? "woki/shaders/depth_masked_skinned"
                                                              : "woki/shaders/depth_masked")
                                             : "woki/shaders/depth_skinned"},
            .label = masked_depth
                         ? (skinned_depth ? "Woki Masked Skinned Depth" : "Woki Masked Depth")
                         : "Woki Skinned Depth",
            .sources = {{.stage = ShaderStage::Vertex,
                .entry_point = "vertex_main",
                .source_path = (root_ / (file + ".wgsl")).generic_string()}},
            .interface = {.uses_object_transform = true, .uses_skinning = skinned_depth},
            .hot_reload = true,
        };
        if (masked_depth) {
            result.sources.push_back({.stage = ShaderStage::Fragment,
                .entry_point = "fragment_main",
                .source_path = result.sources.front().source_path});
            result.interface.parameters = {
                {.name = material_parameters::kBaseColor, .type = ShaderValueType::Float32x4},
                {.name = material_parameters::kAlphaCutoff, .type = ShaderValueType::Float32},
            };
            result.interface.resources = {
                {.name = material_parameters::kBaseColorTexture,
                    .type = ShaderResourceType::Texture2D,
                    .group = 1,
                    .binding = 1},
                {.name = material_parameters::kSampler,
                    .type = ShaderResourceType::Sampler,
                    .group = 1,
                    .binding = 2},
            };
        }
        return result;
    }
    const SurfaceShaderInfo info = SurfaceInfo(shader);
    const std::string source_path = (root_ / (std::string(info.file) + ".wgsl")).generic_string();
    ShaderDesc result{
        .asset_id = AssetId{info.asset},
        .label = std::string(info.label),
        .sources =
            {
                {.stage = ShaderStage::Vertex,
                    .entry_point = "vertex_main",
                    .source_path = source_path},
                {.stage = ShaderStage::Fragment,
                    .entry_point = "fragment_main",
                    .source_path = source_path},
            },
        .interface = StandardInterface(info.pbr, info.skinned, info.shadows, info.environment),
        .hot_reload = true,
    };
    if (info.textured) {
        AddTexturedPbrInterface(result.interface);
    }
    return result;
}

Result<ShaderHandle> StandardShaderLibrary::Load(
    const StandardShader shader, ShaderManager& manager) const {
    return manager.Create(Describe(shader));
}

} // namespace woki::gfx
