#include <woki/gfx/feature/depth_prepass_feature.hpp>

#include <cmath>

namespace woki::gfx {

Result<void> Validate(const DepthPrepassFeatureDesc& desc) {
    if (desc.format == rhi::TextureFormat::Undefined) {
        return Err(
            ErrorCode::GraphicsInvalidFormat, "Depth prepass requires a depth texture format");
    }
    if (!std::isfinite(desc.clear_depth) || desc.clear_depth < 0.0F || desc.clear_depth > 1.0F) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Depth prepass clear value must be finite and normalized");
    }
    return Ok();
}

struct DepthPrepassFeature::PassData final {
    ResolvedDrawList draws{};
    StandardDrawBindings* bindings{nullptr};
};

DepthPrepassFeature::DepthPrepassFeature(const DepthPrepassFeatureDesc& desc, MeshManager& meshes,
    MaterialManager& materials, MaterialPipelineResolver& material_pipelines,
    PipelineManager& pipelines, GpuResourceManager& resources, StandardDrawBindings& bindings)
    : desc_(desc), meshes_(&meshes), materials_(&materials),
      material_pipelines_(&material_pipelines), pipelines_(&pipelines), resources_(&resources),
      bindings_(&bindings) {}

DepthPrepassFeature::~DepthPrepassFeature() = default;

std::string_view DepthPrepassFeature::Name() const noexcept { return "Depth prepass"; }

Result<void> DepthPrepassFeature::AddPasses(
    RenderGraph& graph, RenderGraphBlackboard& blackboard, const RenderFeatureContext& context) {
    if (auto validation = Validate(desc_); !validation) {
        return validation;
    }
    if (blackboard.Contains(render_outputs::kDepth)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Depth prepass must be the first main depth producer");
    }

    const RenderTargetSignature targets{.depth_format = desc_.format};
    auto prepared = PrepareDraws(context.opaque_queue, RenderPassClass::DepthOnly, targets,
        *meshes_, *materials_, *material_pipelines_);
    if (!prepared) {
        return Err(prepared.error());
    }
    auto resolved = ResolveDraws(*prepared, *pipelines_, *resources_);
    if (!resolved) {
        return Err(resolved.error());
    }

    auto depth = graph.AddTransientTexture({
        .label = "Main depth",
        .format = desc_.format,
        .usage = rhi::TextureUsage::RenderAttachment | rhi::TextureUsage::TextureBinding,
        .extent = rhi::ExtentMode::Swapchain(),
    });
    if (!depth) {
        return Err(depth.error());
    }
    if (auto published = blackboard.Publish(render_outputs::kDepth, *depth); !published) {
        return published;
    }

    const u64 view_scope = bindings_->SetView(context.view);
    for (auto& draw : resolved->draws) {
        draw.view_scope = view_scope;
    }
    if (auto bindings = bindings_->Prepare(*resolved); !bindings) {
        return bindings;
    }
    active_frame_ = std::make_shared<PassData>(PassData{
        .draws = std::move(*resolved),
        .bindings = bindings_,
    });
    auto pass = graph.AddPass({
        .label = "Main depth prepass",
        .resources = {{.resource = *depth, .access = GraphAccess::Write}},
        .depth = GraphDepthOutput{.resource = *depth,
            .config = {.load = rhi::LoadOp::Clear,
                .store = rhi::StoreOp::Store,
                .clear = desc_.clear_depth,
                .write = true}},
        .execute = [this](rhi::RenderPassContext& context) -> Result<void> {
            const auto frame = active_frame_;
            if (!frame) {
                return Err(ErrorCode::InvalidState, "Depth prepass has no active frame data");
            }
            auto encoded = EncodePreparedDraws(context.encoder(), frame->draws, frame->bindings);
            return encoded ? Ok() : Err(encoded.error());
        },
    });
    return pass ? Ok() : Err(pass.error());
}

} // namespace woki::gfx
