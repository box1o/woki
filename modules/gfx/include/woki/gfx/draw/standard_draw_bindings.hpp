#pragma once

#include "../graph/render_feature.hpp"
#include "../material/standard_material_resources.hpp"
#include "../resource/frame_uniform_buffer.hpp"
#include "../shader/shader_manager.hpp"
#include "draw_encoder.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace woki::gfx {

struct StandardDrawBindingsDesc final {
    std::optional<u32> transform_immediate_offset{};
    StandardMaterialResources defaults{};
};

class StandardDrawBindings final : public DrawBindingEncoder {
public:
    StandardDrawBindings(rhi::Device& device, GpuResourceManager& resources, ShaderManager& shaders,
        FrameUniformBuffer& uniforms, StandardDrawBindingsDesc desc = {});
    ~StandardDrawBindings() override;

    StandardDrawBindings(const StandardDrawBindings&) = delete;
    StandardDrawBindings& operator=(const StandardDrawBindings&) = delete;
    StandardDrawBindings(StandardDrawBindings&&) = delete;
    StandardDrawBindings& operator=(StandardDrawBindings&&) = delete;

    [[nodiscard]] Result<void> Prepare(const ResolvedDrawList& draws) override;
    void Encode(rhi::RenderPassEncoder& pass, const ResolvedDraw& draw, u32 draw_index) override;

    [[nodiscard]] std::size_t MaterialBindingCount() const noexcept;
    [[nodiscard]] Result<void> SetLighting(std::span<const std::byte> data);
    void SetView(const RenderView& view) noexcept;
    void ClearLighting() noexcept;
    void Clear() noexcept;

private:
    struct GroupBinding final {
        u32 group{0};
        scope<rhi::BindGroup> binding{};
    };

    struct MaterialBinding final {
        const rhi::RenderPipeline* pipeline{nullptr};
        MaterialHandle material{};
        std::vector<GroupBinding> groups{};
    };

    struct ObjectBinding final {
        const rhi::RenderPipeline* pipeline{nullptr};
        RenderObjectHandle object{};
        u32 group{0};
        scope<rhi::BindGroup> binding{};
    };

    [[nodiscard]] Result<MaterialBinding> BuildBinding(const ResolvedDraw& draw);
    [[nodiscard]] MaterialBinding* Find(
        const rhi::RenderPipeline* pipeline, MaterialHandle material) noexcept;
    [[nodiscard]] ObjectBinding* Find(std::vector<ObjectBinding>& bindings,
        const rhi::RenderPipeline* pipeline, RenderObjectHandle object) noexcept;

    rhi::Device* device_{nullptr};
    GpuResourceManager* resources_{nullptr};
    ShaderManager* shaders_{nullptr};
    FrameUniformBuffer* uniforms_{nullptr};
    StandardDrawBindingsDesc desc_{};
    std::vector<MaterialBinding> materials_{};
    std::vector<ObjectBinding> objects_{};
    std::vector<ObjectBinding> skins_{};
    std::optional<UniformBufferSlice> lighting_{};
    RenderView view_{};
    u64 snapshot_sequence_{0};
};

} // namespace woki::gfx
