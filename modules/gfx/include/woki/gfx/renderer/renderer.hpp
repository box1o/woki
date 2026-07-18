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
    u32 render_passes{0};
    u32 shaders_reloaded{0};
    u32 shader_reload_failures{0};
    u64 uniform_bytes{0};
    bool graph_rebuilt{false};
};

struct ShaderHotReloadReport final {
    u32 changed_files{0};
    u32 affected_shaders{0};
    u32 reloaded_shaders{0};
    u32 rebuilt_shader_sets{0};
    std::vector<Error> failures{};
};

struct RendererDiagnostics final {
    u64 frames_rendered{0};
    u64 graph_rebuilds{0};
    u64 total_draws{0};
    std::optional<RenderFrameResult> last_frame{};
    ShaderHotReloadReport last_hot_reload{};
};

class Renderer final {
public:
    Renderer(rhi::Device& device, GpuResourceManager& resources, ShaderManager& shaders,
        PipelineManager& pipelines, MaterialManager& materials, RenderScene& scene,
        RenderFeatureRegistry& features, FrameUniformBuffer& uniforms) noexcept;

    [[nodiscard]] Result<RenderFrameResult> Render(const RenderFrameDesc& desc);
    [[nodiscard]] ShaderHotReloadReport ProcessHotReload();

    void InvalidateGraph() noexcept;
    [[nodiscard]] bool HasCompiledGraph() const noexcept;
    [[nodiscard]] u64 LastSubmission() const noexcept;
    [[nodiscard]] const RendererDiagnostics& Diagnostics() const noexcept;

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
    RendererDiagnostics diagnostics_{};
};

} // namespace woki::gfx
