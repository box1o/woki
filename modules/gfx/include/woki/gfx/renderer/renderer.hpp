#pragma once

#include "../frame/render_frame_planner.hpp"
#include "../graph/rhi_render_graph.hpp"
#include "../material/material_manager.hpp"
#include "../pipeline/pipeline_manager.hpp"
#include "../resource/frame_uniform_buffer.hpp"
#include "../shader/shader_manager.hpp"

#include <optional>

namespace woki::gfx {

struct RenderFrameDesc final {
    u64 frame_number{0};
    u64 submission{0};
    u64 completed_submission{0};
    u64 layer_mask{~0ULL};
    u32 width{0};
    u32 height{0};
    rhi::TextureView* output{nullptr};
};

struct RenderFrameResult final {
    u64 snapshot_sequence{0};
    u64 submission{0};
    u32 opaque_draws{0};
    u32 transparent_draws{0};
    bool graph_rebuilt{false};
};

class Renderer final {
public:
    Renderer(rhi::Device& device, GpuResourceManager& resources, ShaderManager& shaders,
        PipelineManager& pipelines, MaterialManager& materials, RenderScene& scene,
        RenderFeatureRegistry& features, FrameUniformBuffer& uniforms) noexcept;

    [[nodiscard]] Result<RenderFrameResult> Render(const RenderFrameDesc& desc);

    void InvalidateGraph() noexcept;
    [[nodiscard]] bool HasCompiledGraph() const noexcept;
    [[nodiscard]] u64 LastSubmission() const noexcept;

private:
    void Collect(u64 completed_submission);

    rhi::Device* device_{nullptr};
    GpuResourceManager* resources_{nullptr};
    ShaderManager* shaders_{nullptr};
    PipelineManager* pipelines_{nullptr};
    MaterialManager* materials_{nullptr};
    RenderFeatureRegistry* features_{nullptr};
    FrameUniformBuffer* uniforms_{nullptr};
    RenderFramePlanner planner_;
    std::optional<ExecutableRenderGraph> graph_{};
    u64 graph_revision_{0};
    u64 last_submission_{0};
};

} // namespace woki::gfx
