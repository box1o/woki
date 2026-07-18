#pragma once

#include <string>
#include <vector>

#include <woki/core.hpp>

namespace woki::gfx {

enum class ShaderValueType : u8 {
    Bool = 0,
    Int32,
    Uint32,
    Float32,
    Float32x2,
    Float32x3,
    Float32x4,
    Float32x4x4,
};

enum class ShaderResourceType : u8 {
    Texture2D = 0,
    TextureCube,
    Sampler,
    ComparisonSampler,
};

struct ShaderParameterDesc final {
    StringId name{};
    ShaderValueType type{ShaderValueType::Float32};
    bool required{false};
};

struct ShaderResourceBindingDesc final {
    StringId name{};
    ShaderResourceType type{ShaderResourceType::Texture2D};
    u32 group{0};
    u32 binding{0};
    bool required{false};
};

struct ShaderInterfaceDesc final {
    bool uses_object_transform{false};
    u32 object_group{0};
    u32 object_binding{0};
    bool uses_skinning{false};
    u32 skin_group{3};
    u32 skin_binding{0};
    u32 parameter_group{1};
    u32 parameter_binding{0};
    bool uses_lighting{false};
    u32 lighting_group{0};
    u32 lighting_binding{0};
    bool uses_shadows{false};
    u32 shadow_group{0};
    u32 shadow_data_binding{1};
    u32 shadow_texture_binding{2};
    u32 shadow_sampler_binding{3};
    std::vector<ShaderParameterDesc> parameters{};
    std::vector<ShaderResourceBindingDesc> resources{};
};

[[nodiscard]] Result<void> Validate(const ShaderInterfaceDesc& desc);

} // namespace woki::gfx
