#pragma once

#include "gpu_resource.hpp"
#include "resource_metadata.hpp"
#include "resource_types.hpp"

#include <memory>

namespace woki::gfx {

class GpuResourceManager final {
public:
    explicit GpuResourceManager(rhi::Device& device);
    ~GpuResourceManager();

    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;
    GpuResourceManager(GpuResourceManager&&) = delete;
    GpuResourceManager& operator=(GpuResourceManager&&) = delete;

    [[nodiscard]] Result<BufferHandle> CreateBuffer(const BufferResourceDesc& desc);
    [[nodiscard]] Result<TextureHandle> CreateTexture(const TextureResourceDesc& desc);
    [[nodiscard]] Result<SamplerHandle> CreateSampler(const SamplerResourceDesc& desc);

    [[nodiscard]] BufferHandle FindBuffer(AssetId asset_id) const noexcept;
    [[nodiscard]] TextureHandle FindTexture(AssetId asset_id) const noexcept;
    [[nodiscard]] SamplerHandle FindSampler(AssetId asset_id) const noexcept;

    [[nodiscard]] rhi::Buffer* Resolve(BufferHandle handle) noexcept;
    [[nodiscard]] const rhi::Buffer* Resolve(BufferHandle handle) const noexcept;
    [[nodiscard]] rhi::Texture* Resolve(TextureHandle handle) noexcept;
    [[nodiscard]] const rhi::Texture* Resolve(TextureHandle handle) const noexcept;
    [[nodiscard]] rhi::TextureView* ResolveView(TextureHandle handle) noexcept;
    [[nodiscard]] const rhi::TextureView* ResolveView(TextureHandle handle) const noexcept;
    [[nodiscard]] rhi::Sampler* Resolve(SamplerHandle handle) noexcept;
    [[nodiscard]] const rhi::Sampler* Resolve(SamplerHandle handle) const noexcept;

    [[nodiscard]] const ResourceMetadata* Metadata(BufferHandle handle) const noexcept;
    [[nodiscard]] const ResourceMetadata* Metadata(TextureHandle handle) const noexcept;
    [[nodiscard]] const ResourceMetadata* Metadata(SamplerHandle handle) const noexcept;

    [[nodiscard]] bool Destroy(BufferHandle handle);
    [[nodiscard]] bool Destroy(TextureHandle handle);
    [[nodiscard]] bool Destroy(SamplerHandle handle);

    [[nodiscard]] bool Retire(BufferHandle handle, u64 after_submission);
    [[nodiscard]] bool Retire(TextureHandle handle, u64 after_submission);
    [[nodiscard]] bool Retire(SamplerHandle handle, u64 after_submission);
    void Collect(u64 completed_submission);

    [[nodiscard]] std::size_t BufferCount() const noexcept;
    [[nodiscard]] std::size_t TextureCount() const noexcept;
    [[nodiscard]] std::size_t SamplerCount() const noexcept;
    [[nodiscard]] std::size_t RetiredCount() const noexcept;

    void Clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace woki::gfx
