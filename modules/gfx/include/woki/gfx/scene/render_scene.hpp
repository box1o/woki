#pragma once

#include "../lighting/light.hpp"
#include "../resource/resource_types.hpp"

#include <memory>
#include <span>
#include <vector>

#include <woki/math/math.hpp>

namespace woki::gfx {

class MaterialManager;
class MeshManager;

enum class DrawPhase : u8 {
    Opaque = 0,
    Transparent,
};

struct RenderObjectDesc final {
    MeshHandle mesh{};
    std::vector<MaterialHandle> materials{};
    math::mat4f transform{math::mat4f::identity()};
    u64 layer_mask{~0ULL};
    f32 sort_depth{0.0F};
    bool visible{true};
    bool casts_shadows{true};
};

struct ExtractedObject final {
    RenderObjectHandle object{};
    MeshHandle mesh{};
    std::vector<MaterialHandle> materials{};
    math::mat4f transform{math::mat4f::identity()};
    u64 layer_mask{~0ULL};
    f32 sort_depth{0.0F};
    bool casts_shadows{true};
};

struct DrawPacket final {
    RenderObjectHandle object{};
    MeshHandle mesh{};
    MaterialHandle material{};
    u32 submesh{0};
    u32 first_index{0};
    u32 index_count{0};
    i32 base_vertex{0};
    DrawPhase phase{DrawPhase::Opaque};
    f32 sort_depth{0.0F};
};

struct RenderSnapshot final {
    u64 sequence{0};
    std::vector<ExtractedObject> objects{};
    std::vector<DrawPacket> draws{};
    std::vector<ExtractedLight> lights{};
};

void SortDrawPackets(std::span<DrawPacket> draws);

class RenderScene final {
public:
    RenderScene(MeshManager& meshes, MaterialManager& materials);
    ~RenderScene();

    RenderScene(const RenderScene&) = delete;
    RenderScene& operator=(const RenderScene&) = delete;
    RenderScene(RenderScene&&) = delete;
    RenderScene& operator=(RenderScene&&) = delete;

    [[nodiscard]] Result<RenderObjectHandle> Create(const RenderObjectDesc& desc);
    [[nodiscard]] Result<void> Update(RenderObjectHandle object, const RenderObjectDesc& desc);
    [[nodiscard]] bool Remove(RenderObjectHandle object);
    [[nodiscard]] bool Contains(RenderObjectHandle object) const noexcept;
    [[nodiscard]] const RenderObjectDesc* Resolve(RenderObjectHandle object) const noexcept;

    [[nodiscard]] Result<LightHandle> CreateLight(const LightDesc& desc);
    [[nodiscard]] Result<void> UpdateLight(LightHandle light, const LightDesc& desc);
    [[nodiscard]] bool RemoveLight(LightHandle light);
    [[nodiscard]] const LightDesc* ResolveLight(LightHandle light) const noexcept;

    [[nodiscard]] Result<RenderSnapshot> Extract(u64 layer_mask = ~0ULL);

    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t LightCount() const noexcept;
    void Clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace woki::gfx
