#pragma once

#include "../resource/resource_id.hpp"
#include "../resource/resource_types.hpp"

#include <concepts>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include <woki/core.hpp>
#include <woki/math/math.hpp>

namespace woki::gfx {

enum class MaterialModel : u8 {
    Custom = 0,
    Unlit,
    PhysicallyBased,
};

enum class MaterialBlendMode : u8 {
    Opaque = 0,
    Masked,
    Translucent,
};

using MaterialParameterValue = std::variant<bool, i32, u32, f32, math::vec2f, math::vec3f,
    math::vec4f, math::mat4f, TextureHandle, SamplerHandle>;

class MaterialParameters final {
public:
    template <typename T>
        requires std::constructible_from<MaterialParameterValue, T>
    void Set(const StringId name, T&& value) {
        values_.insert_or_assign(name, MaterialParameterValue(std::forward<T>(value)));
    }

    [[nodiscard]] bool Remove(const StringId name) { return values_.erase(name) != 0; }

    [[nodiscard]] bool Contains(const StringId name) const noexcept {
        return values_.contains(name);
    }

    template <typename T> [[nodiscard]] const T* TryGet(const StringId name) const noexcept {
        const auto iterator = values_.find(name);
        if (iterator == values_.end()) {
            return nullptr;
        }

        return std::get_if<T>(&iterator->second);
    }

    [[nodiscard]] const auto& Values() const noexcept { return values_; }

    [[nodiscard]] bool Empty() const noexcept { return values_.empty(); }

private:
    std::unordered_map<StringId, MaterialParameterValue> values_{};
};

struct MaterialDesc final {
    AssetId asset_id{};
    std::string label{};
    MaterialModel model{MaterialModel::PhysicallyBased};
    MaterialBlendMode blend_mode{MaterialBlendMode::Opaque};
    ShaderHandle shader{};
    MaterialParameters parameters{};
    bool double_sided{false};
    bool depth_write{true};
};

struct PbrMaterialDesc final {
    AssetId asset_id{};
    std::string label{};
    math::vec4f base_color{1.0F};
    f32 metallic{0.0F};
    f32 roughness{1.0F};
    math::vec3f emissive{};
    f32 normal_scale{1.0F};
    f32 occlusion_strength{1.0F};
    f32 alpha_cutoff{0.5F};
    TextureHandle base_color_texture{};
    TextureHandle metallic_roughness_texture{};
    TextureHandle normal_texture{};
    TextureHandle occlusion_texture{};
    TextureHandle emissive_texture{};
    SamplerHandle sampler{};
    MaterialBlendMode blend_mode{MaterialBlendMode::Opaque};
    bool double_sided{false};
};

namespace material_parameters {
inline const StringId kBaseColor{"base_color"};
inline const StringId kMetallic{"metallic"};
inline const StringId kRoughness{"roughness"};
inline const StringId kEmissive{"emissive"};
inline const StringId kNormalScale{"normal_scale"};
inline const StringId kOcclusionStrength{"occlusion_strength"};
inline const StringId kAlphaCutoff{"alpha_cutoff"};
inline const StringId kBaseColorTexture{"base_color_texture"};
inline const StringId kMetallicRoughnessTexture{"metallic_roughness_texture"};
inline const StringId kNormalTexture{"normal_texture"};
inline const StringId kOcclusionTexture{"occlusion_texture"};
inline const StringId kEmissiveTexture{"emissive_texture"};
inline const StringId kSampler{"sampler"};
} // namespace material_parameters

[[nodiscard]] MaterialDesc MakePbrMaterial(
    const PbrMaterialDesc& desc, ShaderHandle standard_shader = {});

} // namespace woki::gfx
