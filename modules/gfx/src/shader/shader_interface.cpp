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
    for (const auto& resource : desc.resources) {
        if (resource.name.Empty()) {
            return Err(ErrorCode::ValidationNullValue, "Shader interface resource requires a name");
        }
        if (!names.insert(resource.name).second) {
            return Err(ErrorCode::ValidationInvalidState, "Shader interface names must be unique");
        }
        const u64 slot = (static_cast<u64>(resource.group) << 32U) | resource.binding;
        if (!bindings.insert(slot).second) {
            return Err(ErrorCode::ValidationInvalidState,
                "Shader interface resource bindings must be unique");
        }
    }
    return Ok();
}

} // namespace woki::gfx
