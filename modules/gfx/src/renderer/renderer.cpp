#include <woki/gfx/renderer/renderer.hpp>

#include <woki/gfx/feature/forward_render_feature.hpp>

namespace woki::gfx {

Renderer::Renderer(rhi::Device& device, GpuResourceManager& resources, ShaderManager& shaders,
    PipelineManager& pipelines, MaterialManager& materials, RenderScene& scene,
    RenderFeatureRegistry& features, FrameUniformBuffer& uniforms) noexcept
    : device_(&device), resources_(&resources), shaders_(&shaders), pipelines_(&pipelines),
      materials_(&materials), features_(&features), uniforms_(&uniforms),
      planner_(scene, features) {}

void Renderer::Collect(const u64 completed_submission) {
    resources_->Collect(completed_submission);
    shaders_->Collect(completed_submission);
    pipelines_->Collect(completed_submission);
    materials_->Collect(completed_submission);
}

Result<RenderFrameResult> Renderer::Render(const RenderFrameDesc& desc) {
    if (desc.width == 0 || desc.height == 0 || desc.output == nullptr || desc.submission == 0) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Renderer frame requires dimensions, an output view, and a submission identity");
    }
    if (desc.submission <= last_submission_ || desc.completed_submission > desc.submission) {
        return Err(
            ErrorCode::ValidationInvalidState, "Renderer submission identities must be monotonic");
    }

    Collect(desc.completed_submission);
    if (auto begun = uniforms_->BeginFrame(desc.frame_number, desc.completed_submission); !begun) {
        return Err(begun.error());
    }
    const auto abort = [this]() { uniforms_->AbortFrame(); };

    auto plan = planner_.Prepare(desc.layer_mask);
    if (!plan) {
        abort();
        return Err(plan.error());
    }

    bool graph_rebuilt = false;
    if (!graph_ || graph_revision_ != features_->Revision()) {
        auto executable = CompileRhiRenderGraph(plan->feature_graph.graph,
            plan->feature_graph.compiled, *device_, *resources_, desc.width, desc.height);
        if (!executable) {
            abort();
            return Err(executable.error());
        }
        graph_ = std::move(*executable);
        graph_revision_ = features_->Revision();
        graph_rebuilt = true;
    }

    if (auto flushed = uniforms_->Flush(); !flushed) {
        abort();
        return Err(flushed.error());
    }

    const GraphResource output = plan->feature_graph.blackboard.Find(render_outputs::kColor);
    if (!output) {
        abort();
        return Err(
            ErrorCode::FailedToAcquireResource, "Renderer graph did not publish its color output");
    }
    auto frame = graph_->BeginFrame(*device_, desc.width, desc.height);
    if (!frame) {
        abort();
        return Err(frame.error());
    }
    if (auto bound = frame->Bind(output, *desc.output); !bound) {
        abort();
        return Err(bound.error());
    }
    if (auto executed = frame->Execute(); !executed) {
        abort();
        return Err(executed.error());
    }
    if (auto submitted = uniforms_->MarkSubmitted(desc.submission); !submitted) {
        return Err(submitted.error());
    }
    last_submission_ = desc.submission;

    return Ok(RenderFrameResult{
        .snapshot_sequence = plan->snapshot.sequence,
        .submission = desc.submission,
        .opaque_draws = static_cast<u32>(plan->opaque_queue.draws.size()),
        .transparent_draws = static_cast<u32>(plan->transparent_queue.draws.size()),
        .graph_rebuilt = graph_rebuilt,
    });
}

void Renderer::InvalidateGraph() noexcept { graph_revision_ = 0; }
bool Renderer::HasCompiledGraph() const noexcept { return graph_.has_value(); }
u64 Renderer::LastSubmission() const noexcept { return last_submission_; }

} // namespace woki::gfx
