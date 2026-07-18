#include <woki/gfx/feature/forward_render_feature.hpp>

namespace woki::gfx {

struct ForwardRenderFeature::PassData final {
    ResolvedDrawList opaque{};
    ResolvedDrawList transparent{};
    StandardDrawBindings* bindings{nullptr};
    bool shadows{false};
};

ForwardRenderFeature::ForwardRenderFeature(const ForwardRenderFeatureDesc& desc,
    MeshManager& meshes, MaterialManager& materials, MaterialPipelineResolver& material_pipelines,
    PipelineManager& pipelines, GpuResourceManager& resources, StandardDrawBindings& bindings)
    : desc_(desc), meshes_(&meshes), materials_(&materials),
      material_pipelines_(&material_pipelines), pipelines_(&pipelines), resources_(&resources),
      bindings_(&bindings) {}

ForwardRenderFeature::~ForwardRenderFeature() = default;

std::string_view ForwardRenderFeature::Name() const noexcept { return "Forward renderer"; }

Result<void> ForwardRenderFeature::AddPasses(
    RenderGraph& graph, RenderGraphBlackboard& blackboard, const RenderFeatureContext& context) {
    if (desc_.targets.color_formats.size() != 1 || !desc_.targets.depth_format) {
        return Err(ErrorCode::ValidationInvalidState,
            "Forward renderer requires one color target and one depth target");
    }
    if (desc_.targets.sample_count == 0) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Forward renderer sample count must be nonzero");
    }

    GraphResource color = blackboard.Find(render_outputs::kColor);
    if (color) {
        const GraphResourceDesc* published_color = graph.Resource(color);
        if (published_color == nullptr || published_color->kind != GraphResourceKind::Texture ||
            (published_color->origin == GraphResourceOrigin::Transient &&
                (published_color->transient.format != desc_.targets.color_formats.front() ||
                    published_color->transient.sample_count != 1))) {
            return Err(ErrorCode::GraphicsInvalidFormat,
                "Published color is incompatible with the forward target signature");
        }
    }
    if (!color) {
        auto created = desc_.offscreen_color ? graph.AddTransientTexture({
                                                   .label = "Forward color",
                                                   .format = desc_.targets.color_formats.front(),
                                                   .usage = rhi::TextureUsage::RenderAttachment |
                                                            rhi::TextureUsage::TextureBinding,
                                                   .extent = rhi::ExtentMode::Swapchain(),
                                               })
                                             : graph.AddPerFrameTexture("Forward color");
        if (!created) {
            return Err(created.error());
        }
        color = *created;
        if (auto published = blackboard.Publish(render_outputs::kColor, color); !published) {
            return published;
        }
    }
    GraphResource render_color = color;
    const bool multisampled = desc_.targets.sample_count > 1;
    if (multisampled) {
        auto created = graph.AddTransientTexture({
            .label = "Forward multisampled color",
            .format = desc_.targets.color_formats.front(),
            .usage = rhi::TextureUsage::RenderAttachment,
            .sample_count = desc_.targets.sample_count,
            .extent = rhi::ExtentMode::Swapchain(),
        });
        if (!created) {
            return Err(created.error());
        }
        render_color = *created;
    }

    GraphResource depth = blackboard.Find(render_outputs::kDepth);
    const bool depth_prepass = static_cast<bool>(depth);
    if (depth_prepass) {
        const GraphResourceDesc* published_depth = graph.Resource(depth);
        if (published_depth == nullptr || published_depth->kind != GraphResourceKind::Texture ||
            (published_depth->origin == GraphResourceOrigin::Transient &&
                (published_depth->transient.format != *desc_.targets.depth_format ||
                    published_depth->transient.sample_count != desc_.targets.sample_count))) {
            return Err(ErrorCode::GraphicsInvalidFormat,
                "Published main depth is incompatible with the forward target signature");
        }
    }
    if (!depth) {
        auto created = graph.AddTransientTexture({
            .label = "Forward depth",
            .format = *desc_.targets.depth_format,
            .usage = rhi::TextureUsage::RenderAttachment | rhi::TextureUsage::TextureBinding,
            .sample_count = desc_.targets.sample_count,
            .extent = rhi::ExtentMode::Swapchain(),
        });
        if (!created) {
            return Err(created.error());
        }
        depth = *created;
        if (auto published = blackboard.Publish(render_outputs::kDepth, depth); !published) {
            return published;
        }
    }

    auto prepared_opaque = PrepareDraws(context.opaque_queue, RenderPassClass::ForwardOpaque,
        desc_.targets, *meshes_, *materials_, *material_pipelines_);
    if (!prepared_opaque) {
        return Err(prepared_opaque.error());
    }
    auto prepared_transparent =
        PrepareDraws(context.transparent_queue, RenderPassClass::ForwardTransparent, desc_.targets,
            *meshes_, *materials_, *material_pipelines_);
    if (!prepared_transparent) {
        return Err(prepared_transparent.error());
    }
    auto opaque = ResolveDraws(*prepared_opaque, *pipelines_, *resources_);
    if (!opaque) {
        return Err(opaque.error());
    }
    auto transparent = ResolveDraws(*prepared_transparent, *pipelines_, *resources_);
    if (!transparent) {
        return Err(transparent.error());
    }

    active_frame_ = std::make_shared<PassData>(PassData{
        .opaque = std::move(*opaque),
        .transparent = std::move(*transparent),
        .bindings = bindings_,
        .shadows = false,
    });
    bindings_->ClearLighting();
    bindings_->ClearShadow();
    bindings_->ClearEnvironment();
    const u64 view_scope = bindings_->SetView(context.view);
    for (auto& draw : active_frame_->opaque.draws) {
        draw.view_scope = view_scope;
    }
    for (auto& draw : active_frame_->transparent.draws) {
        draw.view_scope = view_scope;
    }
    auto lighting =
        PackLighting(context.snapshot.lights, desc_.ambient_light, desc_.maximum_lights);
    if (!lighting) {
        active_frame_.reset();
        return Err(lighting.error());
    }
    if (auto lighting_binding = bindings_->SetLighting(lighting->bytes); !lighting_binding) {
        active_frame_.reset();
        return Err(lighting_binding.error());
    }
    const GraphResource shadow_depth = blackboard.Find(render_outputs::kShadowDepth);
    const auto* shadow_data = blackboard.FindData<ShadowFrameData>(render_outputs::kShadowData);
    if (static_cast<bool>(shadow_depth) != (shadow_data != nullptr)) {
        active_frame_.reset();
        return Err(ErrorCode::ValidationInvalidState,
            "Forward renderer requires shadow depth and metadata together");
    }
    if (shadow_data) {
        if (auto shadow_binding = bindings_->SetShadow(*shadow_data); !shadow_binding) {
            active_frame_.reset();
            return shadow_binding;
        }
    }
    if (desc_.environment) {
        if (auto environment_binding = bindings_->SetEnvironment(*desc_.environment);
            !environment_binding) {
            active_frame_.reset();
            return environment_binding;
        }
    }
    ResolvedDrawList all_draws{};
    all_draws.draws.reserve(
        active_frame_->opaque.draws.size() + active_frame_->transparent.draws.size());
    all_draws.draws.insert(all_draws.draws.end(), active_frame_->opaque.draws.begin(),
        active_frame_->opaque.draws.end());
    all_draws.draws.insert(all_draws.draws.end(), active_frame_->transparent.draws.begin(),
        active_frame_->transparent.draws.end());
    if (auto bindings = bindings_->Prepare(all_draws); !bindings) {
        active_frame_.reset();
        return bindings;
    }
    active_frame_->shadows = static_cast<bool>(shadow_depth);

    std::vector<GraphResourceUse> opaque_resources{
        {.resource = render_color, .access = GraphAccess::Write},
        {.resource = depth,
            .access = depth_prepass ? GraphAccess::ReadWrite : GraphAccess::Write},
    };
    std::vector<GraphResourceUse> transparent_resources{
        {.resource = render_color, .access = GraphAccess::ReadWrite},
        {.resource = depth, .access = GraphAccess::ReadWrite},
    };
    if (multisampled) {
        opaque_resources.push_back({.resource = color, .access = GraphAccess::Write});
        transparent_resources.push_back({.resource = color, .access = GraphAccess::Write});
    }

    auto opaque_pass = graph.AddPass({
        .label = "Forward opaque",
        .resources = std::move(opaque_resources),
        .colors = {{
            .resource = render_color,
            .resolve = multisampled ? color : GraphResource{},
            .slot = 0,
            .config = {.load = rhi::LoadOp::Clear,
                .store = rhi::StoreOp::Store,
                .clear = desc_.clear_color},
        }},
        .depth =
            GraphDepthOutput{
                .resource = depth,
                .config = {.load = depth_prepass ? rhi::LoadOp::Load : rhi::LoadOp::Clear,
                    .store = rhi::StoreOp::Store,
                    .clear = desc_.clear_depth,
                    .write = true},
            },
        .samples = shadow_depth ? std::vector<GraphSampleInput>{{.resource = shadow_depth,
                                      .mode = rhi::SampleMode::DepthTexture}}
                                : std::vector<GraphSampleInput>{},
        .execute = [this](rhi::RenderPassContext& pass) -> Result<void> {
            const auto frame = active_frame_;
            if (!frame) {
                return Err(ErrorCode::InvalidState, "Forward opaque pass has no active frame data");
            }
            if (auto prepared = frame->bindings->PrepareFrame(
                    pass, frame->opaque, frame->shadows ? std::optional<u32>{0} : std::nullopt);
                !prepared) {
                return prepared;
            }
            auto encoded = EncodePreparedDraws(pass.encoder(), frame->opaque, frame->bindings);
            return encoded ? Ok() : Err(encoded.error());
        },
    });
    if (!opaque_pass) {
        return Err(opaque_pass.error());
    }

    auto transparent_pass = graph.AddPass({
        .label = "Forward transparent",
        .resources = std::move(transparent_resources),
        .colors = {{
            .resource = render_color,
            .resolve = multisampled ? color : GraphResource{},
            .slot = 0,
            .config = {.load = rhi::LoadOp::Load, .store = rhi::StoreOp::Store},
        }},
        .depth =
            GraphDepthOutput{
                .resource = depth,
                .config = {.load = rhi::LoadOp::Load,
                    .store = rhi::StoreOp::Store,
                    .clear = desc_.clear_depth,
                    .write = false},
            },
        .samples = shadow_depth ? std::vector<GraphSampleInput>{{.resource = shadow_depth,
                                      .mode = rhi::SampleMode::DepthTexture}}
                                : std::vector<GraphSampleInput>{},
        .execute = [this](rhi::RenderPassContext& pass) -> Result<void> {
            const auto frame = active_frame_;
            if (!frame) {
                return Err(
                    ErrorCode::InvalidState, "Forward transparent pass has no active frame data");
            }
            if (auto prepared = frame->bindings->PrepareFrame(pass, frame->transparent,
                    frame->shadows ? std::optional<u32>{0} : std::nullopt);
                !prepared) {
                return prepared;
            }
            auto encoded = EncodePreparedDraws(pass.encoder(), frame->transparent, frame->bindings);
            return encoded ? Ok() : Err(encoded.error());
        },
    });
    if (!transparent_pass) {
        return Err(transparent_pass.error());
    }
    return Ok();
}

} // namespace woki::gfx
