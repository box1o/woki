#include <woki/gfx/feature/post_process_feature.hpp>

#include <array>

namespace woki::gfx {

Result<void> Validate(const PostProcessFeatureDesc& desc) {
    if (!desc.pipeline || !desc.sampler) {
        return Err(
            ErrorCode::ValidationNullValue, "Post-process feature requires a pipeline and sampler");
    }
    if (desc.texture_binding == desc.sampler_binding) {
        return Err(ErrorCode::ValidationInvalidState,
            "Post-process texture and sampler bindings must be unique");
    }
    return Ok();
}

PostProcessFeature::PostProcessFeature(
    const PostProcessFeatureDesc& desc, PipelineManager& pipelines, GpuResourceManager& resources)
    : desc_(desc), pipelines_(&pipelines), resources_(&resources) {}

PostProcessFeature::~PostProcessFeature() = default;

std::string_view PostProcessFeature::Name() const noexcept { return "Post process"; }

Result<void> PostProcessFeature::AddPasses(
    RenderGraph& graph, RenderGraphBlackboard& blackboard, const RenderFeatureContext&) {
    if (auto validation = Validate(desc_); !validation) {
        return validation;
    }
    const GraphResource input = blackboard.Find(render_outputs::kColor);
    if (!input) {
        return Err(ErrorCode::FailedToAcquireResource,
            "Post-process feature requires a published color input");
    }
    const GraphResourceDesc* input_desc = graph.Resource(input);
    if (input_desc == nullptr || input_desc->origin == GraphResourceOrigin::PerFrame) {
        return Err(ErrorCode::ValidationInvalidState,
            "Post-process input must be an offscreen graph texture");
    }
    auto output = graph.AddPerFrameTexture("Post-process output");
    if (!output) {
        return Err(output.error());
    }
    if (auto replaced = blackboard.Replace(render_outputs::kColor, *output); !replaced) {
        return replaced;
    }
    auto pass = graph.AddPass({
        .label = "Post process",
        .resources = {{.resource = input, .access = GraphAccess::Read},
            {.resource = *output, .access = GraphAccess::Write}},
        .colors = {{.resource = *output,
            .slot = 0,
            .config = {.load = rhi::LoadOp::Clear, .store = rhi::StoreOp::Store}}},
        .samples = {{.resource = input, .mode = rhi::SampleMode::ColorTexture}},
        .execute = [this](rhi::RenderPassContext& context) -> Result<void> {
            const rhi::RenderPipeline* pipeline = pipelines_->Resolve(desc_.pipeline);
            rhi::Sampler* sampler = resources_->Resolve(desc_.sampler);
            if (pipeline == nullptr || sampler == nullptr) {
                return Err(ErrorCode::FailedToAcquireResource,
                    "Post-process pipeline or sampler is no longer active");
            }
            auto layout = pipeline->GetBindGroupLayout(desc_.bind_group);
            if (!layout) {
                return Err(ErrorCode::FailedToAcquireResource,
                    "Post-process bind group layout is unavailable");
            }
            const std::array<rhi::BindGroupEntryDesc, 2> entries{{
                {.binding = desc_.texture_binding, .texture_view = &context.sample(0)},
                {.binding = desc_.sampler_binding, .sampler = sampler},
            }};
            auto group = context.device().CreateBindGroup({
                .layout = layout.get(),
                .entries = entries,
                .label = "Post-process inputs",
            });
            if (!group) {
                return Err(group.error());
            }
            context.encoder().SetPipeline(*pipeline);
            context.encoder().SetBindGroup(desc_.bind_group, group->get());
            context.encoder().Draw(3);
            return Ok();
        },
    });
    return pass ? Ok() : Err(pass.error());
}

} // namespace woki::gfx
