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
    u32 parameter_group{1};
    u32 parameter_binding{0};
    std::vector<ShaderParameterDesc> parameters{};
    std::vector<ShaderResourceBindingDesc> resources{};
};

[[nodiscard]] Result<void> Validate(const ShaderInterfaceDesc& desc);

} // namespace woki::gfx
