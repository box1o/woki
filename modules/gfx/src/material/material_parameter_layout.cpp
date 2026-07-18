#include <woki/gfx/material/material_parameter_layout.hpp>

#include <algorithm>
#include <cstring>
#include <unordered_set>

namespace woki::gfx {
namespace {

struct TypeLayout final {
    u32 alignment{0};
    u32 size{0};
};

[[nodiscard]] constexpr TypeLayout LayoutOf(const ShaderValueType type) noexcept {
    switch (type) {
    case ShaderValueType::Bool:
    case ShaderValueType::Int32:
    case ShaderValueType::Uint32:
    case ShaderValueType::Float32:
        return {4, 4};
    case ShaderValueType::Float32x2:
        return {8, 8};
    case ShaderValueType::Float32x3:
        return {16, 12};
    case ShaderValueType::Float32x4:
        return {16, 16};
    case ShaderValueType::Float32x4x4:
        return {16, 64};
    }
    return {};
}

[[nodiscard]] constexpr u32 AlignUp(const u32 value, const u32 alignment) noexcept {
    return (value + alignment - 1U) & ~(alignment - 1U);
}

template <typename T>
[[nodiscard]] Result<void> CopyValue(
    std::span<std::byte> destination, const MaterialParameterValue& value, const char* expected) {
    const auto* typed = std::get_if<T>(&value);
    if (typed == nullptr) {
        return Err(ErrorCode::ValidationInvalidState,
            std::string("Material parameter type does not match ") + expected);
    }
    std::memcpy(destination.data(), typed, sizeof(T));
    return Ok();
}

[[nodiscard]] Result<void> PackValue(std::span<std::byte> destination, const ShaderValueType type,
    const MaterialParameterValue& value) {
    switch (type) {
    case ShaderValueType::Bool: {
        const auto* boolean = std::get_if<bool>(&value);
        if (boolean == nullptr) {
            return Err(
                ErrorCode::ValidationInvalidState, "Material parameter type does not match bool");
        }
        const u32 encoded = *boolean ? 1U : 0U;
        std::memcpy(destination.data(), &encoded, sizeof(encoded));
        return Ok();
    }
    case ShaderValueType::Int32:
        return CopyValue<i32>(destination, value, "int32");
    case ShaderValueType::Uint32:
        return CopyValue<u32>(destination, value, "uint32");
    case ShaderValueType::Float32:
        return CopyValue<f32>(destination, value, "float32");
    case ShaderValueType::Float32x2:
        return CopyValue<math::vec2f>(destination, value, "float32x2");
    case ShaderValueType::Float32x3:
        return CopyValue<math::vec3f>(destination, value, "float32x3");
    case ShaderValueType::Float32x4:
        return CopyValue<math::vec4f>(destination, value, "float32x4");
    case ShaderValueType::Float32x4x4:
        return CopyValue<math::mat4f>(destination, value, "float32x4x4");
    }
    return Err(ErrorCode::ValidationInvalidState, "Unknown shader parameter type");
}

} // namespace

Result<MaterialParameterLayout> BuildMaterialParameterLayout(
    const std::span<const ShaderParameterDesc> parameters) {
    MaterialParameterLayout layout{};
    layout.fields.reserve(parameters.size());
    std::unordered_set<StringId> names{};
    u32 cursor = 0;
    for (const auto& parameter : parameters) {
        if (parameter.name.Empty() || !names.insert(parameter.name).second) {
            return Err(ErrorCode::ValidationInvalidState,
                "Material parameter layout requires unique nonempty names");
        }
        const TypeLayout type = LayoutOf(parameter.type);
        cursor = AlignUp(cursor, type.alignment);
        layout.fields.push_back({
            .name = parameter.name,
            .type = parameter.type,
            .offset = cursor,
            .size = type.size,
            .required = parameter.required,
        });
        cursor += type.size;
    }
    layout.byte_size = AlignUp(cursor, 16);
    return Ok(std::move(layout));
}

Result<PackedMaterialParameters> PackMaterialParameters(
    const MaterialParameterLayout& layout, const MaterialParameters& parameters) {
    PackedMaterialParameters packed{.bytes = std::vector<std::byte>(layout.byte_size)};
    for (const auto& field : layout.fields) {
        const auto iterator = parameters.Values().find(field.name);
        if (iterator == parameters.Values().end()) {
            if (field.required) {
                return Err(
                    ErrorCode::ValidationNullValue, "Required material parameter is missing");
            }
            continue;
        }
        if (static_cast<u64>(field.offset) + field.size > packed.bytes.size()) {
            return Err(
                ErrorCode::ValidationOutOfRange, "Material parameter field exceeds its layout");
        }
        auto destination = std::span(packed.bytes).subspan(field.offset, field.size);
        if (auto result = PackValue(destination, field.type, iterator->second); !result) {
            return Err(result.error());
        }
    }
    return Ok(std::move(packed));
}

Result<void> ValidateMaterialResources(
    const ShaderInterfaceDesc& interface, const MaterialParameters& parameters) {
    for (const auto& binding : interface.resources) {
        const auto iterator = parameters.Values().find(binding.name);
        if (iterator == parameters.Values().end()) {
            if (binding.required) {
                return Err(ErrorCode::ValidationNullValue, "Required material resource is missing");
            }
            continue;
        }
        const bool valid = binding.type == ShaderResourceType::Sampler ||
                                   binding.type == ShaderResourceType::ComparisonSampler
                               ? std::holds_alternative<SamplerHandle>(iterator->second)
                               : std::holds_alternative<TextureHandle>(iterator->second);
        if (!valid) {
            return Err(ErrorCode::ValidationInvalidState,
                "Material resource type does not match its shader binding");
        }
    }
    return Ok();
}

} // namespace woki::gfx
