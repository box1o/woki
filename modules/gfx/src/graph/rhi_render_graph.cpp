#include <woki/gfx/graph/rhi_render_graph.hpp>

namespace woki::gfx {

RhiRenderGraphFrame::RhiRenderGraphFrame(
    rhi::RenderGraphFrame frame, std::vector<rhi::PerFrameSlot> per_frame_slots)
    : frame_(std::move(frame)), per_frame_slots_(std::move(per_frame_slots)) {}

Result<void> RhiRenderGraphFrame::Bind(const GraphResource resource, rhi::TextureView& view) {
    if (!resource || resource.Index() >= per_frame_slots_.size() ||
        !per_frame_slots_[resource.Index()]) {
        return Err(ErrorCode::ValidationInvalidState, "Graph resource is not a per-frame texture");
    }
    frame_.Bind(per_frame_slots_[resource.Index()], &view);
    return Ok();
}

Result<void> RhiRenderGraphFrame::CaptureTimestamps(
    rhi::QuerySet& query_set, rhi::Buffer& resolve_buffer, rhi::Buffer& readback_buffer) {
    return frame_.CaptureTimestamps(query_set, resolve_buffer, readback_buffer);
}

Result<void> RhiRenderGraphFrame::Execute() { return frame_.Execute(); }

ExecutableRenderGraph::ExecutableRenderGraph(scope<rhi::RenderGraph> graph,
    std::vector<rhi::PerFrameSlot> per_frame_slots, const u32 width, const u32 height)
    : graph_(std::move(graph)), per_frame_slots_(std::move(per_frame_slots)), width_(width),
      height_(height) {}

ExecutableRenderGraph::~ExecutableRenderGraph() = default;

Result<RhiRenderGraphFrame> ExecutableRenderGraph::BeginFrame(
    rhi::Device& device, const u32 width, const u32 height) {
    if (width == 0 || height == 0) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Executable render graph frame dimensions must be nonzero");
    }
    if (width != width_ || height != height_) {
        if (auto rebuilt = RebuildForResize(width, height); !rebuilt) {
            return Err(rebuilt.error());
        }
    }
    return Ok(RhiRenderGraphFrame(graph_->BeginFrame(device, width, height), per_frame_slots_));
}

Result<void> ExecutableRenderGraph::RebuildForResize(const u32 width, const u32 height) {
    if (auto result = graph_->RebuildForResize(width, height); !result) {
        return result;
    }
    width_ = width;
    height_ = height;
    return Ok();
}

Result<ExecutableRenderGraph> CompileRhiRenderGraph(const RenderGraph& graph,
    const CompiledRenderGraph& compiled, rhi::Device& device, GpuResourceManager& resources,
    const u32 width, const u32 height) {
    rhi::RenderGraphBuilder builder(device);
    std::vector<rhi::Resource> native_resources(graph.ResourceCount());
    std::vector<rhi::PerFrameSlot> per_frame_slots(graph.ResourceCount());

    for (u32 index = 0; index < graph.ResourceCount(); ++index) {
        const auto* resource = graph.Resource(GraphResource::FromIndex(index));
        WOKI_ASSERT(resource != nullptr);
        if (resource->kind != GraphResourceKind::Texture) {
            return Err(ErrorCode::GraphicsUnsupportedApi,
                "RHI render graph currently supports texture resources only");
        }
        switch (resource->origin) {
        case GraphResourceOrigin::Transient:
            native_resources[index] = builder.Transient(resource->transient);
            break;
        case GraphResourceOrigin::Imported: {
            const auto* handle = std::get_if<TextureHandle>(&resource->imported);
            rhi::Texture* texture = handle == nullptr ? nullptr : resources.Resolve(*handle);
            if (texture == nullptr) {
                return Err(ErrorCode::FailedToAcquireResource,
                    "Imported graph texture is no longer active");
            }
            native_resources[index] = builder.Use(*texture);
            break;
        }
        case GraphResourceOrigin::PerFrame:
            per_frame_slots[index] = builder.PerFrame();
            break;
        }
    }

    for (const auto& scheduled : compiled.passes) {
        const GraphPassDesc* pass = graph.Pass(scheduled.pass);
        if (pass == nullptr || !pass->execute) {
            return Err(ErrorCode::ValidationNullValue, "Executable graph pass requires a callback");
        }
        auto native_pass = builder.AddPass(pass->label);
        for (const auto& color : pass->colors) {
            const auto* resource = graph.Resource(color.resource);
            if (resource == nullptr) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Graph color output references an invalid resource");
            }
            if (resource->origin == GraphResourceOrigin::PerFrame) {
                native_pass.Color(
                    color.slot, per_frame_slots[color.resource.Index()], color.config);
            } else {
                native_pass.Color(
                    color.slot, native_resources[color.resource.Index()], color.config);
            }
        }
        if (pass->depth) {
            const auto* resource = graph.Resource(pass->depth->resource);
            if (resource == nullptr) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Graph depth output references an invalid resource");
            }
            if (resource->origin == GraphResourceOrigin::PerFrame) {
                native_pass.Depth(
                    per_frame_slots[pass->depth->resource.Index()], pass->depth->config);
            } else {
                native_pass.Depth(
                    native_resources[pass->depth->resource.Index()], pass->depth->config);
            }
        }
        for (const auto& sample : pass->samples) {
            const auto* resource = graph.Resource(sample.resource);
            if (resource == nullptr || resource->origin == GraphResourceOrigin::PerFrame) {
                return Err(ErrorCode::GraphicsUnsupportedApi,
                    "Sampled graph input must be a managed graph resource");
            }
            native_pass.Sample(native_resources[sample.resource.Index()], sample.mode);
        }
        const auto callback = pass->execute;
        native_pass.Execute([callback](rhi::RenderPassContext& context) -> Result<void> {
            return callback(context);
        });
    }

    auto native = builder.Compile(width, height);
    if (!native) {
        return Err(native.error());
    }
    return Ok(ExecutableRenderGraph(std::move(*native), std::move(per_frame_slots), width, height));
}

} // namespace woki::gfx
