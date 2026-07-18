#include <woki/gfx/renderer/renderer.hpp>

#include <woki/gfx/feature/forward_render_feature.hpp>

#include "gpu_frame_profiler.hpp"

#include <algorithm>

namespace woki::gfx {

Renderer::Renderer(rhi::Device& device, GpuResourceManager& resources, ShaderManager& shaders,
    PipelineManager& pipelines, MaterialManager& materials, RenderScene& scene,
    RenderFeatureRegistry& features, FrameUniformBuffer& uniforms)
    : device_(&device), resources_(&resources), shaders_(&shaders), pipelines_(&pipelines),
      materials_(&materials), features_(&features), uniforms_(&uniforms), planner_(scene, features),
      gpu_profiler_(std::make_unique<detail::GpuFrameProfiler>(device)) {
    diagnostics_.gpu_timing_supported = gpu_profiler_->Supported();
    diagnostics_.gpu_timing_active = gpu_profiler_->Active();
    diagnostics_.gpu_timing_initialization_error = gpu_profiler_->InitializationError();
}

Renderer::~Renderer() = default;

void Renderer::Collect(const u64 completed_submission) {
    resources_->Collect(completed_submission);
    shaders_->Collect(completed_submission);
    pipelines_->Collect(completed_submission);
    materials_->Collect(completed_submission);
}

ShaderHotReloadReport Renderer::ProcessHotReload() {
    ShaderHotReloadReport report{};
    ShaderReloadBatch changes = shaders_->PollHotReload();
    report.changed_files = static_cast<u32>(changes.changes.size());
    report.affected_shaders = static_cast<u32>(changes.shaders.size());

    const auto changed = [&changes](const ShaderHandle shader) {
        return std::ranges::find(changes.shaders, shader) != changes.shaders.end();
    };
    const auto retries = pending_pipeline_rebuilds_;
    for (const ShaderHandle shader : retries) {
        if (changed(shader)) {
            continue;
        }
        ++report.retried_pipeline_sets;
        if (auto rebuilt = pipelines_->RebuildForShader(shader, last_submission_); !rebuilt) {
            report.failures.push_back(rebuilt.error());
            continue;
        }
        RemovePipelineRebuild(shader);
        ++report.rebuilt_shader_sets;
    }

    for (const ShaderHandle shader : changes.shaders) {
        if (auto reloaded = shaders_->Reload(shader, last_submission_); !reloaded) {
            report.failures.push_back(reloaded.error());
            continue;
        }
        ++report.reloaded_shaders;
        if (auto rebuilt = pipelines_->RebuildForShader(shader, last_submission_); !rebuilt) {
            QueuePipelineRebuild(shader);
            report.failures.push_back(rebuilt.error());
            continue;
        }
        RemovePipelineRebuild(shader);
        ++report.rebuilt_shader_sets;
    }
    report.pending_pipeline_sets = static_cast<u32>(pending_pipeline_rebuilds_.size());
    diagnostics_.last_hot_reload = report;
    return report;
}

void Renderer::QueuePipelineRebuild(const ShaderHandle shader) {
    if (std::ranges::find(pending_pipeline_rebuilds_, shader) == pending_pipeline_rebuilds_.end()) {
        pending_pipeline_rebuilds_.push_back(shader);
    }
}

void Renderer::RemovePipelineRebuild(const ShaderHandle shader) {
    std::erase(pending_pipeline_rebuilds_, shader);
}

Result<RenderFrameResult> Renderer::Render(const RenderFrameDesc& desc) {
    const auto frame_start = Clock::Now();
    if (desc.width == 0 || desc.height == 0 || desc.output == nullptr || desc.submission == 0) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Renderer frame requires dimensions, an output view, and a submission identity");
    }
    if (desc.submission <= last_submission_ || desc.completed_submission > desc.submission) {
        return Err(
            ErrorCode::ValidationInvalidState, "Renderer submission identities must be monotonic");
    }

    if (const auto gpu_timing = gpu_profiler_->Poll(desc.completed_submission)) {
        ++diagnostics_.gpu_timing_samples;
        diagnostics_.last_gpu_timing_submission = gpu_timing->submission;
        diagnostics_.last_gpu_duration_ns = gpu_timing->duration_ns;
    }

    const auto maintenance_start = Clock::Now();
    Collect(desc.completed_submission);
    const ShaderHotReloadReport hot_reload = ProcessHotReload();
    const auto maintenance_end = Clock::Now();
    if (auto begun = uniforms_->BeginFrame(desc.frame_number, desc.completed_submission); !begun) {
        return Err(begun.error());
    }
    const auto abort = [this]() { uniforms_->AbortFrame(); };

    const auto planning_start = Clock::Now();
    auto plan = planner_.Prepare(desc.view, desc.layer_mask);
    if (!plan) {
        abort();
        return Err(plan.error());
    }
    const auto planning_end = Clock::Now();

    bool graph_rebuilt = false;
    const auto graph_compile_start = Clock::Now();
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
    const auto graph_compile_end = Clock::Now();

    const auto upload_start = Clock::Now();
    if (auto flushed = uniforms_->Flush(); !flushed) {
        abort();
        return Err(flushed.error());
    }
    const auto upload_end = Clock::Now();

    const GraphResource output = plan->feature_graph.blackboard.Find(render_outputs::kColor);
    if (!output) {
        abort();
        return Err(
            ErrorCode::FailedToAcquireResource, "Renderer graph did not publish its color output");
    }
    const auto execution_start = Clock::Now();
    auto frame = graph_->BeginFrame(*device_, desc.width, desc.height);
    if (!frame) {
        abort();
        return Err(frame.error());
    }
    if (auto bound = frame->Bind(output, *desc.output); !bound) {
        abort();
        return Err(bound.error());
    }
    auto gpu_capture = gpu_profiler_->Capture(*frame);
    if (!gpu_capture) {
        abort();
        return Err(gpu_capture.error());
    }
    if (auto executed = frame->Execute(); !executed) {
        gpu_profiler_->CancelCapture();
        abort();
        return Err(executed.error());
    }
    gpu_profiler_->MarkSubmitted(desc.submission);
    const u64 uniform_bytes = uniforms_->Used();
    if (auto submitted = uniforms_->MarkSubmitted(desc.submission); !submitted) {
        return Err(submitted.error());
    }
    const auto execution_end = Clock::Now();
    last_submission_ = desc.submission;

    const RenderFrameResult result{
        .snapshot_sequence = plan->snapshot.sequence,
        .submission = desc.submission,
        .opaque_draws = static_cast<u32>(plan->opaque_queue.draws.size()),
        .transparent_draws = static_cast<u32>(plan->transparent_queue.draws.size()),
        .render_passes = static_cast<u32>(plan->feature_graph.compiled.passes.size()),
        .shaders_reloaded = hot_reload.reloaded_shaders,
        .shader_reload_failures = static_cast<u32>(hot_reload.failures.size()),
        .uniform_bytes = uniform_bytes,
        .graph_rebuilt = graph_rebuilt,
        .gpu_timing_captured = *gpu_capture,
    };
    ++diagnostics_.frames_rendered;
    diagnostics_.graph_rebuilds += graph_rebuilt ? 1 : 0;
    diagnostics_.total_draws += static_cast<u64>(result.opaque_draws) + result.transparent_draws;
    diagnostics_.last_frame = result;
    diagnostics_.last_timings = {
        .maintenance_us = Clock::ToMicroseconds(maintenance_end - maintenance_start),
        .planning_us = Clock::ToMicroseconds(planning_end - planning_start),
        .graph_compile_us = Clock::ToMicroseconds(graph_compile_end - graph_compile_start),
        .upload_us = Clock::ToMicroseconds(upload_end - upload_start),
        .execution_us = Clock::ToMicroseconds(execution_end - execution_start),
        .total_us = Clock::ToMicroseconds(execution_end - frame_start),
    };
    diagnostics_.resources = {
        .buffers = resources_->BufferCount(),
        .textures = resources_->TextureCount(),
        .samplers = resources_->SamplerCount(),
        .shaders = shaders_->Size(),
        .pipelines = pipelines_->Size(),
        .materials = materials_->Size(),
        .retired = resources_->RetiredCount() + shaders_->RetiredCount() +
                   pipelines_->RetiredCount() + materials_->RetiredCount(),
    };
    return Ok(result);
}

void Renderer::InvalidateGraph() noexcept { graph_revision_ = 0; }
bool Renderer::HasCompiledGraph() const noexcept { return graph_.has_value(); }
u64 Renderer::LastSubmission() const noexcept { return last_submission_; }
const RendererDiagnostics& Renderer::Diagnostics() const noexcept { return diagnostics_; }

} // namespace woki::gfx
