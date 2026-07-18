#include <woki/gfx/material/material.hpp>

namespace woki::gfx {

MaterialDesc MakePbrMaterial(const PbrMaterialDesc& desc, const ShaderHandle standard_shader) {
    MaterialDesc material{};
    material.asset_id = desc.asset_id;
    material.label = desc.label;
    material.model = MaterialModel::PhysicallyBased;
    material.blend_mode = desc.blend_mode;
    material.shader = standard_shader;
    material.double_sided = desc.double_sided;
    material.depth_write = desc.blend_mode != MaterialBlendMode::Translucent;

    material.parameters.Set(material_parameters::kBaseColor, desc.base_color);
    material.parameters.Set(material_parameters::kMetallic, desc.metallic);
    material.parameters.Set(material_parameters::kRoughness, desc.roughness);
    material.parameters.Set(material_parameters::kEmissive, desc.emissive);
    material.parameters.Set(material_parameters::kNormalScale, desc.normal_scale);
    material.parameters.Set(material_parameters::kOcclusionStrength, desc.occlusion_strength);
    material.parameters.Set(material_parameters::kAlphaCutoff, desc.alpha_cutoff);

    if (desc.base_color_texture) {
        material.parameters.Set(material_parameters::kBaseColorTexture, desc.base_color_texture);
    }
    if (desc.metallic_roughness_texture) {
        material.parameters.Set(
            material_parameters::kMetallicRoughnessTexture, desc.metallic_roughness_texture);
    }
    if (desc.normal_texture) {
        material.parameters.Set(material_parameters::kNormalTexture, desc.normal_texture);
    }
    if (desc.occlusion_texture) {
        material.parameters.Set(material_parameters::kOcclusionTexture, desc.occlusion_texture);
    }
    if (desc.emissive_texture) {
        material.parameters.Set(material_parameters::kEmissiveTexture, desc.emissive_texture);
    }
    if (desc.sampler) {
        material.parameters.Set(material_parameters::kSampler, desc.sampler);
    }

    return material;
}

Result<void> Validate(const MaterialDesc& desc) {
    if (!desc.shader) {
        return Err(ErrorCode::ValidationNullValue, "Material requires a shader");
    }
    if (desc.blend_mode == MaterialBlendMode::Masked &&
        !desc.parameters.Contains(material_parameters::kAlphaCutoff)) {
        return Err(ErrorCode::ValidationNullValue, "Masked material requires an alpha cutoff");
    }

    const auto validate_unit = [&](const StringId name, const char* label) -> Result<void> {
        const f32* value = desc.parameters.TryGet<f32>(name);
        if (value != nullptr && (*value < 0.0F || *value > 1.0F)) {
            return Err(ErrorCode::ValidationOutOfRange,
                std::string(label) + " must be in the [0, 1] range");
        }
        return Ok();
    };
    if (auto result = validate_unit(material_parameters::kMetallic, "Metallic"); !result) {
        return result;
    }
    if (auto result = validate_unit(material_parameters::kRoughness, "Roughness"); !result) {
        return result;
    }
    if (auto result = validate_unit(material_parameters::kOcclusionStrength, "Occlusion strength");
        !result) {
        return result;
    }
    return validate_unit(material_parameters::kAlphaCutoff, "Alpha cutoff");
}

} // namespace woki::gfx
