#include <woki/gfx/material/material_manager.hpp>

#include "../resource/resource_registry.hpp"

#include <woki/gfx/resource/gpu_resource_manager.hpp>
#include <woki/gfx/shader/shader_manager.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace woki::gfx {
namespace {

[[nodiscard]] Result<void> ValidateReferences(
    const MaterialDesc& desc, const ShaderManager& shaders, const GpuResourceManager& resources) {
    if (shaders.Description(desc.shader) == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Material shader handle is not active");
    }
    for (const auto& [name, value] : desc.parameters.Values()) {
        static_cast<void>(name);
        if (const auto* texture = std::get_if<TextureHandle>(&value);
            texture != nullptr && *texture && resources.Resolve(*texture) == nullptr) {
            return Err(ErrorCode::FailedToAcquireResource, "Material texture handle is not active");
        }
        if (const auto* sampler = std::get_if<SamplerHandle>(&value);
            sampler != nullptr && *sampler && resources.Resolve(*sampler) == nullptr) {
            return Err(ErrorCode::FailedToAcquireResource, "Material sampler handle is not active");
        }
    }
    return Ok();
}

} // namespace

class MaterialManager::Impl final {
public:
    Impl(ShaderManager& shaders, GpuResourceManager& resources)
        : shaders_(&shaders), resources_(&resources) {}

    ShaderManager* shaders_{nullptr};
    GpuResourceManager* resources_{nullptr};
    detail::ResourceRegistry<MaterialDesc, MaterialTag> materials{};

    struct RetiredMaterial final {
        u64 after_submission{0};
        MaterialDesc desc{};
    };
    std::vector<RetiredMaterial> retired{};
};

MaterialManager::MaterialManager(ShaderManager& shaders, GpuResourceManager& resources)
    : impl_(std::make_unique<Impl>(shaders, resources)) {}

MaterialManager::~MaterialManager() { Clear(); }

Result<MaterialHandle> MaterialManager::Create(const MaterialDesc& desc) {
    if (auto validation = Validate(desc); !validation) {
        return Err(validation.error());
    }
    if (auto references = ValidateReferences(desc, *impl_->shaders_, *impl_->resources_);
        !references) {
        return Err(references.error());
    }
    if (const MaterialHandle existing = Find(desc.asset_id); existing) {
        return Ok(existing);
    }
    return Ok(impl_->materials.Create(desc.asset_id, desc.label, ResourceState::Resident, desc));
}

Result<void> MaterialManager::Update(const MaterialHandle material, const MaterialDesc& replacement,
    const u64 retire_after_submission) {
    auto* entry = impl_->materials.TryGet(material);
    if (entry == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Material handle is not active");
    }
    if (replacement.asset_id && replacement.asset_id != entry->metadata.asset_id) {
        return Err(
            ErrorCode::ValidationInvalidState, "Material updates cannot change asset identity");
    }
    if (auto validation = Validate(replacement); !validation) {
        return Err(validation.error());
    }
    if (auto references = ValidateReferences(replacement, *impl_->shaders_, *impl_->resources_);
        !references) {
        return Err(references.error());
    }
    impl_->retired.push_back({retire_after_submission, std::move(entry->value)});
    entry->value = replacement;
    entry->value.asset_id = entry->metadata.asset_id;
    entry->metadata.version = entry->metadata.version.Next();
    entry->metadata.state = ResourceState::Resident;
    return Ok();
}

Result<void> MaterialManager::SetParameter(const MaterialHandle material, const StringId name,
    MaterialParameterValue value, const u64 retire_after_submission) {
    const MaterialDesc* current = Resolve(material);
    if (current == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Material handle is not active");
    }
    MaterialDesc replacement = *current;
    replacement.parameters.Set(name, std::move(value));
    return Update(material, replacement, retire_after_submission);
}

MaterialHandle MaterialManager::Find(const AssetId asset_id) const noexcept {
    return impl_->materials.Find(asset_id);
}

const MaterialDesc* MaterialManager::Resolve(const MaterialHandle material) const noexcept {
    return impl_->materials.TryGetValue(material);
}

Result<MaterialDesc> MaterialManager::Snapshot(const MaterialHandle material) const {
    const MaterialDesc* desc = Resolve(material);
    if (desc == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Material handle is not active");
    }
    return Ok(*desc);
}

const ResourceMetadata* MaterialManager::Metadata(const MaterialHandle material) const noexcept {
    const auto* entry = impl_->materials.TryGet(material);
    return entry == nullptr ? nullptr : &entry->metadata;
}

bool MaterialManager::Destroy(const MaterialHandle material) {
    return impl_->materials.Remove(material);
}

bool MaterialManager::Retire(const MaterialHandle material, const u64 after_submission) {
    auto* entry = impl_->materials.TryGet(material);
    if (entry == nullptr) {
        return false;
    }
    impl_->retired.push_back({after_submission, std::move(entry->value)});
    return impl_->materials.Remove(material);
}

void MaterialManager::Collect(const u64 completed_submission) {
    std::erase_if(impl_->retired, [completed_submission](const auto& retired) {
        return retired.after_submission <= completed_submission;
    });
}

std::size_t MaterialManager::Size() const noexcept { return impl_->materials.Size(); }
std::size_t MaterialManager::RetiredCount() const noexcept { return impl_->retired.size(); }

void MaterialManager::Clear() {
    impl_->materials.Clear();
    Collect(std::numeric_limits<u64>::max());
}

} // namespace woki::gfx
