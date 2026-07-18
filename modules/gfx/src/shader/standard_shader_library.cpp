#include <woki/gfx/shader/standard_shader_library.hpp>

#include <woki/gfx/shader/shader_manager.hpp>

#include <utility>

namespace woki::gfx {
namespace {

[[nodiscard]] ShaderInterfaceDesc StandardInterface(const bool lighting, const bool skinning) {
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

} // namespace

StandardShaderLibrary::StandardShaderLibrary(paths::Path shader_root)
    : root_(std::move(shader_root)) {}

const paths::Path& StandardShaderLibrary::Root() const noexcept { return root_; }

ShaderDesc StandardShaderLibrary::Describe(const StandardShader shader) const {
    const bool pbr = shader != StandardShader::Unlit;
    const bool skinned = shader == StandardShader::PbrSkinned;
    const std::string name = skinned ? "pbr_skinned_forward" : (pbr ? "pbr_forward" : "unlit");
    const std::string source_path = (root_ / (name + ".wgsl")).generic_string();
    return {
        .asset_id = AssetId{skinned ? "woki/shaders/pbr_skinned"
                                    : (pbr ? "woki/shaders/pbr" : "woki/shaders/unlit")},
        .label = skinned ? "Woki PBR Skinned" : (pbr ? "Woki PBR" : "Woki Unlit"),
        .sources =
            {
                {.stage = ShaderStage::Vertex,
                    .entry_point = "vertex_main",
                    .source_path = source_path},
                {.stage = ShaderStage::Fragment,
                    .entry_point = "fragment_main",
                    .source_path = source_path},
            },
        .interface = StandardInterface(pbr, skinned),
        .hot_reload = true,
    };
}

Result<ShaderHandle> StandardShaderLibrary::Load(
    const StandardShader shader, ShaderManager& manager) const {
    return manager.Create(Describe(shader));
}

} // namespace woki::gfx
