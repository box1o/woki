#pragma once

#include "../shader/shader_interface.hpp"
#include "material.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace woki::gfx {

struct MaterialParameterField final {
    StringId name{};
    ShaderValueType type{ShaderValueType::Float32};
    u32 offset{0};
    u32 size{0};
    bool required{false};
};

struct MaterialParameterLayout final {
    std::vector<MaterialParameterField> fields{};
    u32 byte_size{0};
};

struct PackedMaterialParameters final {
    std::vector<std::byte> bytes{};
};

[[nodiscard]] Result<MaterialParameterLayout> BuildMaterialParameterLayout(
    std::span<const ShaderParameterDesc> parameters);
[[nodiscard]] Result<PackedMaterialParameters> PackMaterialParameters(
    const MaterialParameterLayout& layout, const MaterialParameters& parameters);
[[nodiscard]] Result<void> ValidateMaterialResources(
    const ShaderInterfaceDesc& interface, const MaterialParameters& parameters);

} // namespace woki::gfx
