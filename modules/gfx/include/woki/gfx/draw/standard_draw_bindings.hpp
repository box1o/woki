#pragma once

#include "../resource/frame_uniform_buffer.hpp"
#include "../shader/shader_manager.hpp"
#include "draw_encoder.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace woki::gfx {

struct StandardDrawBindingsDesc final {
    std::optional<u32> transform_immediate_offset{};
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
        u32 group{0};
        scope<rhi::BindGroup> binding{};
    };

    [[nodiscard]] Result<MaterialBinding> BuildBinding(const ResolvedDraw& draw);
    [[nodiscard]] MaterialBinding* Find(
        const rhi::RenderPipeline* pipeline, MaterialHandle material) noexcept;

    rhi::Device* device_{nullptr};
    GpuResourceManager* resources_{nullptr};
    ShaderManager* shaders_{nullptr};
    FrameUniformBuffer* uniforms_{nullptr};
    StandardDrawBindingsDesc desc_{};
    std::vector<MaterialBinding> materials_{};
    std::vector<std::optional<ObjectBinding>> objects_{};
    std::optional<UniformBufferSlice> lighting_{};
};

} // namespace woki::gfx
