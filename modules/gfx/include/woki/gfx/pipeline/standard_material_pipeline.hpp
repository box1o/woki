#pragma once

#include "material_pipeline_resolver.hpp"
#include "pipeline_manager.hpp"

namespace woki::gfx {

struct StandardMaterialPipelineDesc final {
    AssetId asset_id{};
    std::string label{};
    ShaderHandle shader{};
    MaterialModel model{MaterialModel::PhysicallyBased};
    MaterialBlendMode blend_mode{MaterialBlendMode::Opaque};
    RenderPassClass pass{RenderPassClass::ForwardOpaque};
    RenderTargetSignature targets{};
    StringId vertex_layout{};
    std::vector<VertexBufferLayout> vertex_buffers{};
    bool double_sided{false};
    bool depth_write{true};
};

struct StandardMaterialPipeline final {
    GraphicsPipelineDesc graphics{};
    MaterialPipelineKey key{};
};

[[nodiscard]] Result<StandardMaterialPipeline> BuildStandardMaterialPipeline(
    const StandardMaterialPipelineDesc& desc);
[[nodiscard]] Result<PipelineHandle> CreateStandardMaterialPipeline(
    const StandardMaterialPipelineDesc& desc, PipelineManager& pipelines,
    MaterialPipelineResolver& resolver);

} // namespace woki::gfx
