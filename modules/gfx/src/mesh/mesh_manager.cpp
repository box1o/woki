#include <woki/gfx/mesh/mesh_manager.hpp>

#include "../resource/resource_registry.hpp"

#include <woki/gfx/resource/gpu_resource_manager.hpp>

#include <limits>
#include <utility>

namespace woki::gfx {
namespace {

struct MeshRecord final {
    MeshDesc desc{};
    std::vector<BufferHandle> vertex_buffers{};
    BufferHandle index_buffer{};
    u32 vertex_count{0};
};

[[nodiscard]] constexpr u64 IndexSize(const rhi::IndexFormat format) noexcept {
    switch (format) {
    case rhi::IndexFormat::Uint16:
        return sizeof(u16);
    case rhi::IndexFormat::Uint32:
        return sizeof(u32);
    case rhi::IndexFormat::Undefined:
        break;
    }
    return 0;
}

} // namespace

Result<void> Validate(const MeshDesc& desc) {
    if (desc.vertex_streams.empty()) {
        return Err(ErrorCode::ValidationNullValue, "Mesh requires at least one vertex stream");
    }

    const u32 vertex_count = desc.vertex_streams.front().vertex_count;
    if (vertex_count == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "Mesh vertex count must be greater than zero");
    }
    for (const auto& stream : desc.vertex_streams) {
        if (stream.stride == 0 || stream.vertex_count != vertex_count) {
            return Err(ErrorCode::ValidationInvalidState,
                "Mesh vertex streams require matching counts and nonzero strides");
        }
        const u64 expected_size = stream.stride * static_cast<u64>(stream.vertex_count);
        if (expected_size != stream.data.size()) {
            return Err(ErrorCode::ValidationOutOfRange,
                "Mesh vertex stream size does not match its stride and vertex count");
        }
    }

    const u64 index_size = IndexSize(desc.index_format);
    if (index_size == 0 || desc.index_count == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "Mesh requires a valid nonempty index buffer");
    }
    if (index_size * static_cast<u64>(desc.index_count) != desc.index_data.size()) {
        return Err(
            ErrorCode::ValidationOutOfRange, "Mesh index data size does not match its format");
    }

    for (const auto& submesh : desc.submeshes) {
        if (submesh.index_count == 0 ||
            static_cast<u64>(submesh.first_index) + submesh.index_count > desc.index_count) {
            return Err(ErrorCode::ValidationOutOfRange, "Submesh index range is outside the mesh");
        }
    }
    return Ok();
}

class MeshManager::Impl final {
public:
    explicit Impl(GpuResourceManager& resources) : resources_(&resources) {}

    GpuResourceManager* resources_{nullptr};
    detail::ResourceRegistry<MeshRecord, MeshTag> meshes{};
};

MeshManager::MeshManager(GpuResourceManager& resources)
    : impl_(std::make_unique<Impl>(resources)) {}

MeshManager::~MeshManager() { Clear(); }

Result<MeshHandle> MeshManager::Create(const MeshDesc& desc) {
    if (auto validation = Validate(desc); !validation) {
        return Err(validation.error());
    }
    if (const MeshHandle existing = Find(desc.asset_id); existing) {
        return Ok(existing);
    }

    MeshRecord record{};
    record.desc = desc;
    if (record.desc.submeshes.empty()) {
        record.desc.submeshes.push_back({
            .first_index = 0,
            .index_count = desc.index_count,
        });
    }
    record.vertex_count = desc.vertex_streams.front().vertex_count;
    record.vertex_buffers.reserve(desc.vertex_streams.size());

    const auto cleanup = [&]() {
        for (const BufferHandle buffer : record.vertex_buffers) {
            static_cast<void>(impl_->resources_->Destroy(buffer));
        }
        if (record.index_buffer) {
            static_cast<void>(impl_->resources_->Destroy(record.index_buffer));
        }
    };

    for (std::size_t index = 0; index < desc.vertex_streams.size(); ++index) {
        const auto& stream = desc.vertex_streams[index];
        BufferResourceDesc buffer_desc{};
        buffer_desc.gpu.size = stream.data.size();
        buffer_desc.gpu.usage = rhi::BufferUsage::Vertex | rhi::BufferUsage::CopyDst;
        buffer_desc.gpu.label = desc.label + ".Vertex" + std::to_string(index);
        buffer_desc.initial_data = stream.data;

        auto buffer = impl_->resources_->CreateBuffer(buffer_desc);
        if (!buffer) {
            cleanup();
            return Err(buffer.error());
        }
        record.vertex_buffers.push_back(*buffer);
    }

    BufferResourceDesc index_desc{};
    index_desc.gpu.size = desc.index_data.size();
    index_desc.gpu.usage = rhi::BufferUsage::Index | rhi::BufferUsage::CopyDst;
    index_desc.gpu.label = desc.label + ".Index";
    index_desc.initial_data = desc.index_data;
    auto index_buffer = impl_->resources_->CreateBuffer(index_desc);
    if (!index_buffer) {
        cleanup();
        return Err(index_buffer.error());
    }
    record.index_buffer = *index_buffer;

    if (!desc.retain_cpu_copy) {
        for (auto& stream : record.desc.vertex_streams) {
            stream.data.clear();
            stream.data.shrink_to_fit();
        }
        record.desc.index_data.clear();
        record.desc.index_data.shrink_to_fit();
    }

    return Ok(impl_->meshes.Create(
        desc.asset_id, desc.label, ResourceState::Resident, std::move(record)));
}

MeshHandle MeshManager::Find(const AssetId asset_id) const noexcept {
    return impl_->meshes.Find(asset_id);
}

Result<MeshView> MeshManager::GetView(const MeshHandle mesh) const {
    const auto* record = impl_->meshes.TryGetValue(mesh);
    if (record == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Mesh handle is not active");
    }
    return Ok(MeshView{
        .vertex_buffers = record->vertex_buffers,
        .index_buffer = record->index_buffer,
        .index_format = record->desc.index_format,
        .vertex_count = record->vertex_count,
        .index_count = record->desc.index_count,
        .submeshes = record->desc.submeshes,
    });
}

const ResourceMetadata* MeshManager::Metadata(const MeshHandle mesh) const noexcept {
    const auto* entry = impl_->meshes.TryGet(mesh);
    return entry == nullptr ? nullptr : &entry->metadata;
}

const MeshDesc* MeshManager::Description(const MeshHandle mesh) const noexcept {
    const auto* record = impl_->meshes.TryGetValue(mesh);
    return record == nullptr ? nullptr : &record->desc;
}

bool MeshManager::Destroy(const MeshHandle mesh) {
    auto* record = impl_->meshes.TryGetValue(mesh);
    if (record == nullptr) {
        return false;
    }
    for (const BufferHandle buffer : record->vertex_buffers) {
        static_cast<void>(impl_->resources_->Destroy(buffer));
    }
    static_cast<void>(impl_->resources_->Destroy(record->index_buffer));
    return impl_->meshes.Remove(mesh);
}

bool MeshManager::Retire(const MeshHandle mesh, const u64 after_submission) {
    auto* record = impl_->meshes.TryGetValue(mesh);
    if (record == nullptr) {
        return false;
    }
    for (const BufferHandle buffer : record->vertex_buffers) {
        static_cast<void>(impl_->resources_->Retire(buffer, after_submission));
    }
    static_cast<void>(impl_->resources_->Retire(record->index_buffer, after_submission));
    return impl_->meshes.Remove(mesh);
}

std::size_t MeshManager::Size() const noexcept { return impl_->meshes.Size(); }

void MeshManager::Clear() {
    std::vector<MeshHandle> handles{};
    handles.reserve(impl_->meshes.Size());
    impl_->meshes.Each([&](const MeshHandle handle, const auto&) { handles.push_back(handle); });
    for (const MeshHandle handle : handles) {
        static_cast<void>(Destroy(handle));
    }
}

} // namespace woki::gfx
