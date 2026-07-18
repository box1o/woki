#include <woki/gfx/pipeline/standard_material_pipeline.hpp>

#include <utility>

namespace woki::gfx {
namespace {

[[nodiscard]] rhi::BlendStateDesc AlphaBlend() noexcept {
    return {
        .color = {.operation = rhi::BlendOperation::Add,
            .src_factor = rhi::BlendFactor::SrcAlpha,
            .dst_factor = rhi::BlendFactor::OneMinusSrcAlpha},
        .alpha = {.operation = rhi::BlendOperation::Add,
            .src_factor = rhi::BlendFactor::One,
            .dst_factor = rhi::BlendFactor::OneMinusSrcAlpha},
    };
}

} // namespace

Result<StandardMaterialPipeline> BuildStandardMaterialPipeline(
    const StandardMaterialPipelineDesc& desc) {
    if (!desc.asset_id || desc.label.empty() || !desc.shader || desc.vertex_layout.Empty()) {
        return Err(ErrorCode::ValidationNullValue,
            "Standard material pipeline requires identity, label, shader, and vertex layout");
    }
    if (desc.pass == RenderPassClass::DepthOnly && desc.blend_mode == MaterialBlendMode::Masked &&
        (!desc.depth_fragment || !desc.implementation_shader)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Masked depth pipelines require a depth fragment implementation shader");
    }
    const bool effective_depth_write =
        desc.depth_write && desc.blend_mode != MaterialBlendMode::Translucent;
    MaterialPipelineKey key{
        .shader = desc.shader,
        .model = desc.model,
        .blend_mode = desc.blend_mode,
        .pass = desc.pass,
        .targets = desc.targets,
        .vertex_layout = desc.vertex_layout,
        .double_sided = desc.double_sided,
        .depth_write = effective_depth_write,
    };
    if (auto validation = Validate(key); !validation) {
        return Err(validation.error());
    }

    GraphicsPipelineDesc graphics{
        .asset_id = desc.asset_id,
        .label = desc.label,
        .shader = desc.implementation_shader ? desc.implementation_shader : desc.shader,
        .vertex_buffers = desc.vertex_buffers,
        .primitive = {.topology = rhi::PrimitiveTopology::TriangleList,
            .front_face = rhi::FrontFace::CCW,
            .cull_mode = desc.double_sided ? rhi::CullMode::None : rhi::CullMode::Back},
        .depth_fragment = desc.depth_fragment,
    };
    if (desc.pass != RenderPassClass::DepthOnly) {
        graphics.color_targets.reserve(desc.targets.color_formats.size());
        for (const auto format : desc.targets.color_formats) {
            graphics.color_targets.push_back({
                .format = format,
                .blend = desc.blend_mode == MaterialBlendMode::Translucent
                             ? std::optional<rhi::BlendStateDesc>{AlphaBlend()}
                             : std::nullopt,
            });
        }
    }
    if (desc.targets.depth_format) {
        graphics.depth_stencil = rhi::DepthStencilStateDesc{
            .format = *desc.targets.depth_format,
            .depth_write_enabled = effective_depth_write,
            .depth_compare = rhi::CompareFunction::LessEqual,
        };
    }
    if (auto validation = Validate(graphics); !validation) {
        return Err(validation.error());
    }
    return Ok(StandardMaterialPipeline{.graphics = std::move(graphics), .key = std::move(key)});
}

Result<PipelineHandle> CreateStandardMaterialPipeline(const StandardMaterialPipelineDesc& desc,
    PipelineManager& pipelines, MaterialPipelineResolver& resolver) {
    auto built = BuildStandardMaterialPipeline(desc);
    if (!built) {
        return Err(built.error());
    }
    auto pipeline = pipelines.Create(built->graphics);
    if (!pipeline) {
        return Err(pipeline.error());
    }
    if (resolver.Contains(built->key)) {
        if (resolver.Resolve(built->key) != *pipeline) {
            return Err(ErrorCode::ValidationInvalidState,
                "Material pipeline key is registered to a different pipeline");
        }
        return pipeline;
    }
    if (auto registered = resolver.Register(built->key, *pipeline); !registered) {
        return Err(registered.error());
    }
    return pipeline;
}

} // namespace woki::gfx
