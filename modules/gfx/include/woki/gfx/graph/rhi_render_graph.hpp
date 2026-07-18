#pragma once

#include "../resource/gpu_resource_manager.hpp"
#include "render_graph.hpp"

namespace woki::gfx {

class RhiRenderGraphFrame final {
public:
    RhiRenderGraphFrame(RhiRenderGraphFrame&&) noexcept = default;
    RhiRenderGraphFrame& operator=(RhiRenderGraphFrame&&) noexcept = default;
    RhiRenderGraphFrame(const RhiRenderGraphFrame&) = delete;
    RhiRenderGraphFrame& operator=(const RhiRenderGraphFrame&) = delete;

    [[nodiscard]] Result<void> Bind(GraphResource resource, rhi::TextureView& view);
    [[nodiscard]] Result<void> Execute();

private:
    friend class ExecutableRenderGraph;

    RhiRenderGraphFrame(
        rhi::RenderGraphFrame frame, std::vector<rhi::PerFrameSlot> per_frame_slots);

    rhi::RenderGraphFrame frame_;
    std::vector<rhi::PerFrameSlot> per_frame_slots_{};
};

class ExecutableRenderGraph final {
public:
    ExecutableRenderGraph(ExecutableRenderGraph&&) noexcept = default;
    ExecutableRenderGraph& operator=(ExecutableRenderGraph&&) noexcept = default;
    ExecutableRenderGraph(const ExecutableRenderGraph&) = delete;
    ExecutableRenderGraph& operator=(const ExecutableRenderGraph&) = delete;
    ~ExecutableRenderGraph();

    [[nodiscard]] RhiRenderGraphFrame BeginFrame(rhi::Device& device, u32 width, u32 height);
    [[nodiscard]] Result<void> RebuildForResize(u32 width, u32 height);

private:
    friend Result<ExecutableRenderGraph> CompileRhiRenderGraph(const RenderGraph&,
        const CompiledRenderGraph&, rhi::Device&, GpuResourceManager&, u32, u32);

    ExecutableRenderGraph(
        scope<rhi::RenderGraph> graph, std::vector<rhi::PerFrameSlot> per_frame_slots);

    scope<rhi::RenderGraph> graph_{};
    std::vector<rhi::PerFrameSlot> per_frame_slots_{};
};

[[nodiscard]] Result<ExecutableRenderGraph> CompileRhiRenderGraph(const RenderGraph& graph,
    const CompiledRenderGraph& compiled, rhi::Device& device, GpuResourceManager& resources,
    u32 width, u32 height);

} // namespace woki::gfx
