#include <woki/gfx/feature/shadow_render_feature.hpp>

#include <cmath>

namespace woki::gfx {

Result<void> Validate(const ShadowRenderFeatureDesc& desc) {
    if (desc.resolution == 0 || desc.format == rhi::TextureFormat::Undefined) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Shadow renderer requires a resolution and depth format");
    }
    if (!std::isfinite(desc.clear_depth) || desc.clear_depth < 0.0F || desc.clear_depth > 1.0F) {
        return Err(
            ErrorCode::ValidationOutOfRange, "Shadow clear depth must be finite and normalized");
    }
    if (!std::isfinite(desc.depth_bias) || desc.depth_bias < 0.0F ||
        !std::isfinite(desc.normal_bias) || desc.normal_bias < 0.0F ||
        !std::isfinite(desc.strength) || desc.strength < 0.0F || desc.strength > 1.0F) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Shadow bias must be nonnegative and strength must be normalized");
    }
    return Ok();
}

struct ShadowRenderFeature::PassData final {
    ResolvedDrawList draws{};
    StandardDrawBindings* bindings{nullptr};
};

ShadowRenderFeature::ShadowRenderFeature(const ShadowRenderFeatureDesc& desc, MeshManager& meshes,
    MaterialManager& materials, MaterialPipelineResolver& material_pipelines,
    PipelineManager& pipelines, GpuResourceManager& resources, StandardDrawBindings& bindings)
    : desc_(desc), meshes_(&meshes), materials_(&materials),
      material_pipelines_(&material_pipelines), pipelines_(&pipelines), resources_(&resources),
      bindings_(&bindings) {}

ShadowRenderFeature::~ShadowRenderFeature() = default;

std::string_view ShadowRenderFeature::Name() const noexcept { return "Shadow renderer"; }

Result<void> ShadowRenderFeature::AddPasses(
    RenderGraph& graph, RenderGraphBlackboard& blackboard, const RenderFeatureContext& context) {
    if (auto validation = Validate(desc_); !validation) {
        return validation;
    }
    if (desc_.light_index >= context.snapshot.lights.size() ||
        !context.snapshot.lights[desc_.light_index].casts_shadows) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Shadow renderer requires a valid shadow-casting light index");
    }
    auto shadow_queue =
        BuildRenderQueue(context.snapshot, {.phase = DrawPhase::Opaque,
                                               .shadow_casters_only = true,
                                               .frustum = desc_.light_view.frustum});
    if (!shadow_queue) {
        return Err(shadow_queue.error());
    }
    const RenderTargetSignature targets{.depth_format = desc_.format};
    auto prepared = PrepareDraws(*shadow_queue, RenderPassClass::DepthOnly, targets, *meshes_,
        *materials_, *material_pipelines_);
    if (!prepared) {
        return Err(prepared.error());
    }
    auto resolved = ResolveDraws(*prepared, *pipelines_, *resources_);
    if (!resolved) {
        return Err(resolved.error());
    }

    GraphResource depth = blackboard.Find(render_outputs::kShadowDepth);
    if (!depth) {
        auto created = graph.AddTransientTexture({
            .label = "Shadow depth",
            .format = desc_.format,
            .usage = rhi::TextureUsage::RenderAttachment | rhi::TextureUsage::TextureBinding,
            .extent = rhi::ExtentMode::Fixed(desc_.resolution, desc_.resolution),
        });
        if (!created) {
            return Err(created.error());
        }
        depth = *created;
        if (auto published = blackboard.Publish(render_outputs::kShadowDepth, depth); !published) {
            return published;
        }
    }
    if (auto published = blackboard.PublishData(render_outputs::kShadowData,
            ShadowFrameData{.light_view_projection = desc_.light_view.view_projection,
                .depth_bias = desc_.depth_bias,
                .normal_bias = desc_.normal_bias,
                .strength = desc_.strength,
                .light_index = desc_.light_index});
        !published) {
        return published;
    }

    const u64 view_scope = bindings_->SetView(desc_.light_view);
    for (auto& draw : resolved->draws) {
        draw.view_scope = view_scope;
    }
    if (auto binding_result = bindings_->Prepare(*resolved); !binding_result) {
        return binding_result;
    }
    active_frame_ = std::make_shared<PassData>(PassData{
        .draws = std::move(*resolved),
        .bindings = bindings_,
    });
    auto pass = graph.AddPass({
        .label = "Shadow depth",
        .resources = {{.resource = depth, .access = GraphAccess::Write}},
        .depth =
            GraphDepthOutput{
                .resource = depth,
                .config = {.load = rhi::LoadOp::Clear,
                    .store = rhi::StoreOp::Store,
                    .clear = desc_.clear_depth,
                    .write = true},
            },
        .execute = [this](rhi::RenderPassContext& context) -> Result<void> {
            const auto frame = active_frame_;
            if (!frame) {
                return Err(ErrorCode::InvalidState, "Shadow pass has no active frame data");
            }
            auto encoded = EncodePreparedDraws(context.encoder(), frame->draws, frame->bindings);
            return encoded ? Ok() : Err(encoded.error());
        },
    });
    return pass ? Ok() : Err(pass.error());
}

} // namespace woki::gfx
