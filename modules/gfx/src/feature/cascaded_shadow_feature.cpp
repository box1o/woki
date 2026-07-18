#include <woki/gfx/feature/cascaded_shadow_feature.hpp>

#include <cmath>
#include <limits>

namespace woki::gfx {

Result<void> Validate(const CascadedShadowFeatureDesc& desc) {
    if (desc.cascades.empty() || desc.cascades.size() > ShadowFrameData::kMaximumCascades) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Cascaded shadows require between one and four cascades");
    }
    if (desc.cascade_resolution == 0 ||
        desc.cascade_resolution > std::numeric_limits<u32>::max() / 2 ||
        desc.format == rhi::TextureFormat::Undefined) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Cascaded shadows require a valid resolution and depth format");
    }
    if (!std::isfinite(desc.clear_depth) || desc.clear_depth < 0.0F ||
        desc.clear_depth > 1.0F || !std::isfinite(desc.depth_bias) || desc.depth_bias < 0.0F ||
        !std::isfinite(desc.normal_bias) || desc.normal_bias < 0.0F ||
        !std::isfinite(desc.strength) || desc.strength < 0.0F || desc.strength > 1.0F) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Cascaded shadow clear, bias, and strength settings are invalid");
    }
    f32 previous_split = 0.0F;
    for (const auto& cascade : desc.cascades) {
        if (!std::isfinite(cascade.far_distance) || cascade.far_distance <= previous_split) {
            return Err(ErrorCode::ValidationOutOfRange,
                "Cascaded shadow distances must be finite, positive, and strictly increasing");
        }
        previous_split = cascade.far_distance;
    }
    return Ok();
}

struct CascadedShadowFeature::PassData final {
    std::vector<ResolvedDrawList> cascades{};
    StandardDrawBindings* bindings{nullptr};
    u32 resolution{0};
    u32 columns{1};
};

CascadedShadowFeature::CascadedShadowFeature(const CascadedShadowFeatureDesc& desc,
    MeshManager& meshes, MaterialManager& materials,
    MaterialPipelineResolver& material_pipelines, PipelineManager& pipelines,
    GpuResourceManager& resources, StandardDrawBindings& bindings)
    : desc_(desc), meshes_(&meshes), materials_(&materials),
      material_pipelines_(&material_pipelines), pipelines_(&pipelines), resources_(&resources),
      bindings_(&bindings) {}

CascadedShadowFeature::~CascadedShadowFeature() = default;

std::string_view CascadedShadowFeature::Name() const noexcept { return "Cascaded shadows"; }

Result<void> CascadedShadowFeature::AddPasses(
    RenderGraph& graph, RenderGraphBlackboard& blackboard, const RenderFeatureContext& context) {
    if (auto validation = Validate(desc_); !validation) {
        return validation;
    }
    if (blackboard.Contains(render_outputs::kShadowDepth) ||
        blackboard.Contains(render_outputs::kShadowData)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Cascaded shadows must be the first shadow producer");
    }
    if (desc_.light_index >= context.snapshot.lights.size() ||
        !context.snapshot.lights[desc_.light_index].casts_shadows) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Cascaded shadows require a valid shadow-casting light index");
    }

    const RenderTargetSignature targets{.depth_format = desc_.format};
    std::vector<ResolvedDrawList> resolved_cascades{};
    resolved_cascades.reserve(desc_.cascades.size());
    ShadowFrameData shadow_data{};
    shadow_data.parameters = {desc_.depth_bias, desc_.normal_bias, desc_.strength, 0.0F};
    shadow_data.light_index = desc_.light_index;
    shadow_data.cascade_count = static_cast<u32>(desc_.cascades.size());

    const u32 columns = desc_.cascades.size() == 1 ? 1U : 2U;
    const u32 rows = desc_.cascades.size() <= 2 ? 1U : 2U;
    const math::vec2f atlas_scale{1.0F / static_cast<f32>(columns),
        1.0F / static_cast<f32>(rows)};
    for (u32 index = 0; index < desc_.cascades.size(); ++index) {
        const auto& cascade = desc_.cascades[index];
        auto queue = BuildRenderQueue(context.snapshot, {.phase = DrawPhase::Opaque,
                                                            .shadow_casters_only = true,
                                                            .frustum = cascade.light_view.frustum});
        if (!queue) {
            return Err(queue.error());
        }
        auto prepared = PrepareDraws(*queue, RenderPassClass::DepthOnly, targets, *meshes_,
            *materials_, *material_pipelines_);
        if (!prepared) {
            return Err(prepared.error());
        }
        auto resolved = ResolveDraws(*prepared, *pipelines_, *resources_);
        if (!resolved) {
            return Err(resolved.error());
        }
        const u64 view_scope = bindings_->SetView(cascade.light_view);
        for (auto& draw : resolved->draws) {
            draw.view_scope = view_scope;
        }
        if (auto binding_result = bindings_->Prepare(*resolved); !binding_result) {
            return binding_result;
        }
        resolved_cascades.push_back(std::move(*resolved));

        const u32 column = index % columns;
        const u32 row = index / columns;
        shadow_data.light_view_projections[index] = cascade.light_view.view_projection;
        shadow_data.atlas_transforms[index] = {atlas_scale.x, atlas_scale.y,
            atlas_scale.x * static_cast<f32>(column), atlas_scale.y * static_cast<f32>(row)};
        shadow_data.split_distances[index] = cascade.far_distance;
    }

    const u32 atlas_width = desc_.cascade_resolution * columns;
    const u32 atlas_height = desc_.cascade_resolution * rows;
    auto depth = graph.AddTransientTexture({
        .label = "Cascaded shadow atlas",
        .format = desc_.format,
        .usage = rhi::TextureUsage::RenderAttachment | rhi::TextureUsage::TextureBinding,
        .extent = rhi::ExtentMode::Fixed(atlas_width, atlas_height),
    });
    if (!depth) {
        return Err(depth.error());
    }
    if (auto published = blackboard.Publish(render_outputs::kShadowDepth, *depth); !published) {
        return published;
    }
    if (auto published = blackboard.PublishData(render_outputs::kShadowData, shadow_data);
        !published) {
        return published;
    }

    active_frame_ = std::make_shared<PassData>(PassData{
        .cascades = std::move(resolved_cascades),
        .bindings = bindings_,
        .resolution = desc_.cascade_resolution,
        .columns = columns,
    });
    auto pass = graph.AddPass({
        .label = "Cascaded shadow atlas",
        .resources = {{.resource = *depth, .access = GraphAccess::Write}},
        .depth = GraphDepthOutput{.resource = *depth,
            .config = {.load = rhi::LoadOp::Clear,
                .store = rhi::StoreOp::Store,
                .clear = desc_.clear_depth,
                .write = true}},
        .execute = [this](rhi::RenderPassContext& context) -> Result<void> {
            const auto frame = active_frame_;
            if (!frame) {
                return Err(ErrorCode::InvalidState,
                    "Cascaded shadow pass has no active frame data");
            }
            for (u32 index = 0; index < frame->cascades.size(); ++index) {
                const u32 x = (index % frame->columns) * frame->resolution;
                const u32 y = (index / frame->columns) * frame->resolution;
                context.encoder().SetViewport(static_cast<f32>(x), static_cast<f32>(y),
                    static_cast<f32>(frame->resolution), static_cast<f32>(frame->resolution),
                    0.0F, 1.0F);
                context.encoder().SetScissorRect(x, y, frame->resolution, frame->resolution);
                auto encoded = EncodePreparedDraws(
                    context.encoder(), frame->cascades[index], frame->bindings);
                if (!encoded) {
                    return Err(encoded.error());
                }
            }
            return Ok();
        },
    });
    return pass ? Ok() : Err(pass.error());
}

} // namespace woki::gfx
