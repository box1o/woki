#pragma once

#include "../resource/resource_metadata.hpp"
#include "mesh.hpp"

#include <memory>

namespace woki::gfx {

class GpuResourceManager;

class MeshManager final {
public:
    explicit MeshManager(GpuResourceManager& resources);
    ~MeshManager();

    MeshManager(const MeshManager&) = delete;
    MeshManager& operator=(const MeshManager&) = delete;
    MeshManager(MeshManager&&) = delete;
    MeshManager& operator=(MeshManager&&) = delete;

    [[nodiscard]] Result<MeshHandle> Create(const MeshDesc& desc);
    [[nodiscard]] MeshHandle Find(AssetId asset_id) const noexcept;
    [[nodiscard]] Result<MeshView> GetView(MeshHandle mesh) const;
    [[nodiscard]] const ResourceMetadata* Metadata(MeshHandle mesh) const noexcept;
    [[nodiscard]] const MeshDesc* Description(MeshHandle mesh) const noexcept;

    [[nodiscard]] bool Destroy(MeshHandle mesh);
    [[nodiscard]] bool Retire(MeshHandle mesh, u64 after_submission);

    [[nodiscard]] std::size_t Size() const noexcept;
    void Clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace woki::gfx
