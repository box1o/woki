#include <woki/gfx/resource/gpu_resource_manager.hpp>

#include "resource_registry.hpp"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>
#include <variant>

namespace woki::gfx {
namespace {

struct BufferRecord final {
    scope<rhi::Buffer> gpu{};
    BufferResourceDesc desc{};
};

struct TextureRecord final {
    scope<rhi::Texture> gpu{};
    scope<rhi::TextureView> view{};
    TextureResourceDesc desc{};
};

struct SamplerRecord final {
    scope<rhi::Sampler> gpu{};
    SamplerResourceDesc desc{};
};

[[nodiscard]] rhi::Extent3D SubresourceExtent(
    const rhi::TextureDesc& texture, const TextureSubresourceData& subresource) noexcept {
    return {
        .width = std::max(1U, texture.size.width >> subresource.mip_level),
        .height = std::max(1U, texture.size.height >> subresource.mip_level),
        .depth_or_array_layers = 1,
    };
}

} // namespace

class GpuResourceManager::Impl final {
public:
    explicit Impl(rhi::Device& device) : device_(&device) {}

    rhi::Device* device_{nullptr};
    detail::ResourceRegistry<BufferRecord, BufferTag> buffers{};
    detail::ResourceRegistry<TextureRecord, TextureTag> textures{};
    detail::ResourceRegistry<SamplerRecord, SamplerTag> samplers{};

    struct RetiredTexture final {
        scope<rhi::Texture> gpu{};
        scope<rhi::TextureView> view{};
    };

    struct RetiredResource final {
        u64 after_submission{0};
        std::variant<scope<rhi::Buffer>, RetiredTexture, scope<rhi::Sampler>> gpu{};
    };
    std::vector<RetiredResource> retired{};
};

GpuResourceManager::GpuResourceManager(rhi::Device& device)
    : impl_(std::make_unique<Impl>(device)) {}

GpuResourceManager::~GpuResourceManager() { Clear(); }

Result<BufferHandle> GpuResourceManager::CreateBuffer(const BufferResourceDesc& desc) {
    if (auto validation = Validate(desc); !validation) {
        return Err(validation.error());
    }
    if (const BufferHandle existing = FindBuffer(desc.asset_id); existing) {
        return Ok(existing);
    }

    auto gpu = impl_->device_->CreateBuffer(desc.gpu);
    if (!gpu) {
        return Err(gpu.error());
    }
    if (!desc.initial_data.empty()) {
        auto uploaded = impl_->device_->GetQueue().WriteBuffer(
            **gpu, 0, desc.initial_data.data(), static_cast<u64>(desc.initial_data.size()));
        if (!uploaded) {
            return Err(uploaded.error());
        }
    }

    BufferResourceDesc stored_desc = desc;
    if (!desc.retain_cpu_copy) {
        stored_desc.initial_data.clear();
        stored_desc.initial_data.shrink_to_fit();
    }
    return Ok(impl_->buffers.Create(desc.asset_id, desc.gpu.label, ResourceState::Resident,
        BufferRecord{.gpu = std::move(*gpu), .desc = std::move(stored_desc)}));
}

Result<TextureHandle> GpuResourceManager::CreateTexture(const TextureResourceDesc& desc) {
    if (auto validation = Validate(desc); !validation) {
        return Err(validation.error());
    }
    if (const TextureHandle existing = FindTexture(desc.asset_id); existing) {
        return Ok(existing);
    }

    auto gpu = impl_->device_->CreateTexture(desc.gpu);
    if (!gpu) {
        return Err(gpu.error());
    }
    for (const auto& subresource : desc.initial_data) {
        const auto native = (*gpu)->GetNativeHandles();
        const rhi::TexelCopyTextureInfo destination{
            .texture = native.resource,
            .mip_level = subresource.mip_level,
            .origin = {.x = 0, .y = 0, .z = subresource.array_layer},
            .aspect = rhi::TextureAspect::All,
        };
        const rhi::TexelCopyBufferLayout layout{
            .offset = 0,
            .bytes_per_row = subresource.bytes_per_row,
            .rows_per_image = subresource.rows_per_image,
        };
        auto uploaded = impl_->device_->GetQueue().WriteTexture(destination,
            subresource.data.data(), static_cast<u64>(subresource.data.size()), layout,
            SubresourceExtent(desc.gpu, subresource));
        if (!uploaded) {
            return Err(uploaded.error());
        }
    }

    TextureResourceDesc stored_desc = desc;
    if (!desc.retain_cpu_copy) {
        stored_desc.initial_data.clear();
        stored_desc.initial_data.shrink_to_fit();
    }
    rhi::TextureViewDesc view_desc = desc.default_view;
    view_desc.label = desc.gpu.label + ".DefaultView";
    auto view = (*gpu)->CreateView(view_desc);
    if (!view) {
        (*gpu)->Destroy();
        return Err(
            ErrorCode::GraphicsTextureCreationFailed, "Failed to create the texture default view");
    }
    return Ok(impl_->textures.Create(desc.asset_id, desc.gpu.label, ResourceState::Resident,
        TextureRecord{
            .gpu = std::move(*gpu), .view = std::move(view), .desc = std::move(stored_desc)}));
}

Result<SamplerHandle> GpuResourceManager::CreateSampler(const SamplerResourceDesc& desc) {
    if (auto validation = Validate(desc); !validation) {
        return Err(validation.error());
    }
    if (const SamplerHandle existing = FindSampler(desc.asset_id); existing) {
        return Ok(existing);
    }

    auto gpu = impl_->device_->CreateSampler(desc.gpu);
    if (!gpu) {
        return Err(gpu.error());
    }
    return Ok(impl_->samplers.Create(desc.asset_id, desc.gpu.label, ResourceState::Resident,
        SamplerRecord{.gpu = std::move(*gpu), .desc = desc}));
}

BufferHandle GpuResourceManager::FindBuffer(const AssetId asset_id) const noexcept {
    return impl_->buffers.Find(asset_id);
}

TextureHandle GpuResourceManager::FindTexture(const AssetId asset_id) const noexcept {
    return impl_->textures.Find(asset_id);
}

SamplerHandle GpuResourceManager::FindSampler(const AssetId asset_id) const noexcept {
    return impl_->samplers.Find(asset_id);
}

rhi::Buffer* GpuResourceManager::Resolve(const BufferHandle handle) noexcept {
    auto* record = impl_->buffers.TryGetValue(handle);
    return record == nullptr ? nullptr : record->gpu.get();
}

const rhi::Buffer* GpuResourceManager::Resolve(const BufferHandle handle) const noexcept {
    const auto* record = impl_->buffers.TryGetValue(handle);
    return record == nullptr ? nullptr : record->gpu.get();
}

rhi::Texture* GpuResourceManager::Resolve(const TextureHandle handle) noexcept {
    auto* record = impl_->textures.TryGetValue(handle);
    return record == nullptr ? nullptr : record->gpu.get();
}

const rhi::Texture* GpuResourceManager::Resolve(const TextureHandle handle) const noexcept {
    const auto* record = impl_->textures.TryGetValue(handle);
    return record == nullptr ? nullptr : record->gpu.get();
}

rhi::TextureView* GpuResourceManager::ResolveView(const TextureHandle handle) noexcept {
    auto* record = impl_->textures.TryGetValue(handle);
    return record == nullptr ? nullptr : record->view.get();
}

const rhi::TextureView* GpuResourceManager::ResolveView(const TextureHandle handle) const noexcept {
    const auto* record = impl_->textures.TryGetValue(handle);
    return record == nullptr ? nullptr : record->view.get();
}

rhi::Sampler* GpuResourceManager::Resolve(const SamplerHandle handle) noexcept {
    auto* record = impl_->samplers.TryGetValue(handle);
    return record == nullptr ? nullptr : record->gpu.get();
}

const rhi::Sampler* GpuResourceManager::Resolve(const SamplerHandle handle) const noexcept {
    const auto* record = impl_->samplers.TryGetValue(handle);
    return record == nullptr ? nullptr : record->gpu.get();
}

const ResourceMetadata* GpuResourceManager::Metadata(const BufferHandle handle) const noexcept {
    const auto* entry = impl_->buffers.TryGet(handle);
    return entry == nullptr ? nullptr : &entry->metadata;
}

const ResourceMetadata* GpuResourceManager::Metadata(const TextureHandle handle) const noexcept {
    const auto* entry = impl_->textures.TryGet(handle);
    return entry == nullptr ? nullptr : &entry->metadata;
}

const ResourceMetadata* GpuResourceManager::Metadata(const SamplerHandle handle) const noexcept {
    const auto* entry = impl_->samplers.TryGet(handle);
    return entry == nullptr ? nullptr : &entry->metadata;
}

bool GpuResourceManager::Destroy(const BufferHandle handle) {
    auto* record = impl_->buffers.TryGetValue(handle);
    if (record == nullptr) {
        return false;
    }
    record->gpu->Destroy();
    return impl_->buffers.Remove(handle);
}

bool GpuResourceManager::Destroy(const TextureHandle handle) {
    auto* record = impl_->textures.TryGetValue(handle);
    if (record == nullptr) {
        return false;
    }
    record->view.reset();
    record->gpu->Destroy();
    return impl_->textures.Remove(handle);
}

bool GpuResourceManager::Destroy(const SamplerHandle handle) {
    return impl_->samplers.Remove(handle);
}

bool GpuResourceManager::Retire(const BufferHandle handle, const u64 after_submission) {
    auto* record = impl_->buffers.TryGetValue(handle);
    if (record == nullptr) {
        return false;
    }
    impl_->retired.push_back({after_submission, std::move(record->gpu)});
    return impl_->buffers.Remove(handle);
}

bool GpuResourceManager::Retire(const TextureHandle handle, const u64 after_submission) {
    auto* record = impl_->textures.TryGetValue(handle);
    if (record == nullptr) {
        return false;
    }
    impl_->retired.push_back({after_submission,
        Impl::RetiredTexture{.gpu = std::move(record->gpu), .view = std::move(record->view)}});
    return impl_->textures.Remove(handle);
}

bool GpuResourceManager::Retire(const SamplerHandle handle, const u64 after_submission) {
    auto* record = impl_->samplers.TryGetValue(handle);
    if (record == nullptr) {
        return false;
    }
    impl_->retired.push_back({after_submission, std::move(record->gpu)});
    return impl_->samplers.Remove(handle);
}

void GpuResourceManager::Collect(const u64 completed_submission) {
    std::erase_if(impl_->retired, [completed_submission](auto& retired) {
        if (retired.after_submission > completed_submission) {
            return false;
        }

        std::visit(
            [](auto& gpu) {
                using Retired = std::remove_cvref_t<decltype(gpu)>;
                if constexpr (std::same_as<Retired, scope<rhi::Buffer>>) {
                    gpu->Destroy();
                } else if constexpr (std::same_as<Retired, Impl::RetiredTexture>) {
                    gpu.view.reset();
                    gpu.gpu->Destroy();
                }
            },
            retired.gpu);
        return true;
    });
}

std::size_t GpuResourceManager::BufferCount() const noexcept { return impl_->buffers.Size(); }

std::size_t GpuResourceManager::TextureCount() const noexcept { return impl_->textures.Size(); }

std::size_t GpuResourceManager::SamplerCount() const noexcept { return impl_->samplers.Size(); }

std::size_t GpuResourceManager::RetiredCount() const noexcept { return impl_->retired.size(); }

void GpuResourceManager::Clear() {
    impl_->buffers.Each([](BufferHandle, auto& entry) { entry.value.gpu->Destroy(); });
    impl_->textures.Each([](TextureHandle, auto& entry) {
        entry.value.view.reset();
        entry.value.gpu->Destroy();
    });
    impl_->buffers.Clear();
    impl_->textures.Clear();
    impl_->samplers.Clear();
    Collect(std::numeric_limits<u64>::max());
}

} // namespace woki::gfx
