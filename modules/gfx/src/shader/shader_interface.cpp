#include <woki/gfx/shader/shader_interface.hpp>

#include <unordered_set>

namespace woki::gfx {

Result<void> Validate(const ShaderInterfaceDesc& desc) {
    std::unordered_set<StringId> names{};
    for (const auto& parameter : desc.parameters) {
        if (parameter.name.Empty()) {
            return Err(
                ErrorCode::ValidationNullValue, "Shader interface parameter requires a name");
        }
        if (!names.insert(parameter.name).second) {
            return Err(ErrorCode::ValidationInvalidState, "Shader interface names must be unique");
        }
    }

    std::unordered_set<u64> bindings{};
    if (desc.uses_object_transform) {
        bindings.insert((static_cast<u64>(desc.object_group) << 32U) | desc.object_binding);
    }
    if (!desc.parameters.empty()) {
        if (desc.uses_object_transform && desc.parameter_group == desc.object_group) {
            return Err(ErrorCode::ValidationInvalidState,
                "Shader object transforms require a dedicated bind group");
        }
        const u64 parameters =
            (static_cast<u64>(desc.parameter_group) << 32U) | desc.parameter_binding;
        if (!bindings.insert(parameters).second) {
            return Err(ErrorCode::ValidationInvalidState,
                "Shader object and parameter bindings cannot overlap");
        }
    }
    if (desc.uses_lighting) {
        if (desc.uses_object_transform && desc.lighting_group == desc.object_group) {
            return Err(ErrorCode::ValidationInvalidState,
                "Shader object transforms require a dedicated bind group");
        }
        const u64 lighting = (static_cast<u64>(desc.lighting_group) << 32U) | desc.lighting_binding;
        if (!bindings.insert(lighting).second) {
            return Err(ErrorCode::ValidationInvalidState,
                "Shader lighting and parameter bindings cannot overlap");
        }
    }
    for (const auto& resource : desc.resources) {
        if (resource.name.Empty()) {
            return Err(ErrorCode::ValidationNullValue, "Shader interface resource requires a name");
        }
        if (!names.insert(resource.name).second) {
            return Err(ErrorCode::ValidationInvalidState, "Shader interface names must be unique");
        }
        const u64 slot = (static_cast<u64>(resource.group) << 32U) | resource.binding;
        if (desc.uses_object_transform && resource.group == desc.object_group) {
            return Err(ErrorCode::ValidationInvalidState,
                "Shader object transforms require a dedicated bind group");
        }
        if (!bindings.insert(slot).second) {
            return Err(ErrorCode::ValidationInvalidState,
                "Shader interface resource bindings must be unique");
        }
    }
    return Ok();
}

} // namespace woki::gfx
