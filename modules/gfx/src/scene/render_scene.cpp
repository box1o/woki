#include <woki/gfx/scene/render_scene.hpp>

#include "../resource/resource_pool.hpp"

#include <woki/gfx/material/material_manager.hpp>
#include <woki/gfx/mesh/mesh_manager.hpp>

#include <algorithm>
#include <tuple>

namespace woki::gfx {
namespace {

[[nodiscard]] Result<void> ValidateObject(
    const RenderObjectDesc& desc, const MeshManager& meshes, const MaterialManager& materials) {
    if (!desc.mesh || desc.materials.empty()) {
        return Err(ErrorCode::ValidationNullValue, "Render object requires a mesh and materials");
    }
    auto mesh = meshes.GetView(desc.mesh);
    if (!mesh) {
        return Err(mesh.error());
    }
    for (const auto& submesh : mesh->submeshes) {
        if (submesh.material_slot >= desc.materials.size()) {
            return Err(ErrorCode::ValidationOutOfRange,
                "Render object does not provide every submesh material slot");
        }
    }
    for (const MaterialHandle material : desc.materials) {
        if (materials.Resolve(material) == nullptr) {
            return Err(ErrorCode::FailedToAcquireResource, "Render object material is not active");
        }
    }
    return Ok();
}

} // namespace

void SortDrawPackets(const std::span<DrawPacket> draws) {
    std::ranges::sort(draws, [](const DrawPacket& lhs, const DrawPacket& rhs) {
        if (lhs.phase != rhs.phase) {
            return lhs.phase < rhs.phase;
        }
        if (lhs.phase == DrawPhase::Transparent && lhs.sort_depth != rhs.sort_depth) {
            return lhs.sort_depth > rhs.sort_depth;
        }
        return std::tuple{lhs.material.Value(), lhs.mesh.Value(), lhs.submesh, lhs.object.Value()} <
               std::tuple{rhs.material.Value(), rhs.mesh.Value(), rhs.submesh, rhs.object.Value()};
    });
}

class RenderScene::Impl final {
public:
    Impl(MeshManager& meshes, MaterialManager& materials)
        : meshes_(&meshes), materials_(&materials) {}

    MeshManager* meshes_{nullptr};
    MaterialManager* materials_{nullptr};
    detail::ResourcePool<RenderObjectDesc, RenderObjectTag> objects{};
    u64 next_sequence{1};
};

RenderScene::RenderScene(MeshManager& meshes, MaterialManager& materials)
    : impl_(std::make_unique<Impl>(meshes, materials)) {}

RenderScene::~RenderScene() = default;

Result<RenderObjectHandle> RenderScene::Create(const RenderObjectDesc& desc) {
    if (auto validation = ValidateObject(desc, *impl_->meshes_, *impl_->materials_); !validation) {
        return Err(validation.error());
    }
    return Ok(impl_->objects.Emplace(desc));
}

Result<void> RenderScene::Update(const RenderObjectHandle object, const RenderObjectDesc& desc) {
    auto* current = impl_->objects.TryGet(object);
    if (current == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Render object handle is not active");
    }
    if (auto validation = ValidateObject(desc, *impl_->meshes_, *impl_->materials_); !validation) {
        return Err(validation.error());
    }
    *current = desc;
    return Ok();
}

bool RenderScene::Remove(const RenderObjectHandle object) { return impl_->objects.Remove(object); }
bool RenderScene::Contains(const RenderObjectHandle object) const noexcept {
    return impl_->objects.Contains(object);
}

const RenderObjectDesc* RenderScene::Resolve(const RenderObjectHandle object) const noexcept {
    return impl_->objects.TryGet(object);
}

Result<RenderSnapshot> RenderScene::Extract(const u64 layer_mask) {
    RenderSnapshot snapshot{.sequence = impl_->next_sequence++};
    snapshot.objects.reserve(impl_->objects.Size());

    Result<void> status = Ok();
    impl_->objects.Each([&](const RenderObjectHandle handle, const RenderObjectDesc& object) {
        if (!status || !object.visible || (object.layer_mask & layer_mask) == 0) {
            return;
        }
        auto mesh = impl_->meshes_->GetView(object.mesh);
        if (!mesh) {
            status = Err(mesh.error());
            return;
        }

        snapshot.objects.push_back({
            .object = handle,
            .mesh = object.mesh,
            .materials = object.materials,
            .transform = object.transform,
            .layer_mask = object.layer_mask,
            .sort_depth = object.sort_depth,
            .casts_shadows = object.casts_shadows,
        });
        for (u32 index = 0; index < mesh->submeshes.size(); ++index) {
            const auto& submesh = mesh->submeshes[index];
            const MaterialHandle material = object.materials[submesh.material_slot];
            const MaterialDesc* material_desc = impl_->materials_->Resolve(material);
            if (material_desc == nullptr) {
                status = Err(ErrorCode::FailedToAcquireResource,
                    "Material became invalid during scene extraction");
                return;
            }
            snapshot.draws.push_back({
                .object = handle,
                .mesh = object.mesh,
                .material = material,
                .submesh = index,
                .first_index = submesh.first_index,
                .index_count = submesh.index_count,
                .base_vertex = submesh.base_vertex,
                .phase = material_desc->blend_mode == MaterialBlendMode::Translucent
                             ? DrawPhase::Transparent
                             : DrawPhase::Opaque,
                .sort_depth = object.sort_depth,
            });
        }
    });
    if (!status) {
        return Err(status.error());
    }
    SortDrawPackets(snapshot.draws);
    return Ok(std::move(snapshot));
}

std::size_t RenderScene::Size() const noexcept { return impl_->objects.Size(); }
void RenderScene::Clear() { impl_->objects.Clear(); }

} // namespace woki::gfx
