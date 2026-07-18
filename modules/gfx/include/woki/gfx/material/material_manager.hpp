#pragma once

#include "../resource/resource_metadata.hpp"
#include "material.hpp"

#include <memory>

namespace woki::gfx {

class GpuResourceManager;
class ShaderManager;

class MaterialManager final {
public:
    MaterialManager(ShaderManager& shaders, GpuResourceManager& resources);
    ~MaterialManager();

    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;
    MaterialManager(MaterialManager&&) = delete;
    MaterialManager& operator=(MaterialManager&&) = delete;

    [[nodiscard]] Result<MaterialHandle> Create(const MaterialDesc& desc);
    [[nodiscard]] Result<void> Update(
        MaterialHandle material, const MaterialDesc& replacement, u64 retire_after_submission);
    [[nodiscard]] Result<void> SetParameter(MaterialHandle material, StringId name,
        MaterialParameterValue value, u64 retire_after_submission);

    template <typename T>
        requires std::constructible_from<MaterialParameterValue, T>
    [[nodiscard]] Result<void> SetParameter(const MaterialHandle material, const StringId name,
        T&& value, const u64 retire_after_submission) {
        return SetParameter(material, name, MaterialParameterValue(std::forward<T>(value)),
            retire_after_submission);
    }

    [[nodiscard]] MaterialHandle Find(AssetId asset_id) const noexcept;
    [[nodiscard]] const MaterialDesc* Resolve(MaterialHandle material) const noexcept;
    [[nodiscard]] Result<MaterialDesc> Snapshot(MaterialHandle material) const;
    [[nodiscard]] const ResourceMetadata* Metadata(MaterialHandle material) const noexcept;

    [[nodiscard]] bool Destroy(MaterialHandle material);
    [[nodiscard]] bool Retire(MaterialHandle material, u64 after_submission);
    void Collect(u64 completed_submission);
    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t RetiredCount() const noexcept;
    void Clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace woki::gfx
