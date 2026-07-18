#pragma once

#include "../draw/standard_draw_bindings.hpp"
#include "../graph/render_feature.hpp"
#include "../graph/render_outputs.hpp"

namespace woki::gfx {

struct DepthPrepassFeatureDesc final {
    rhi::TextureFormat format{rhi::TextureFormat::Depth32Float};
    f32 clear_depth{1.0F};
};

[[nodiscard]] Result<void> Validate(const DepthPrepassFeatureDesc& desc);

class DepthPrepassFeature final : public RenderFeature {
public:
    DepthPrepassFeature(const DepthPrepassFeatureDesc& desc, MeshManager& meshes,
        MaterialManager& materials, MaterialPipelineResolver& material_pipelines,
        PipelineManager& pipelines, GpuResourceManager& resources, StandardDrawBindings& bindings);
    ~DepthPrepassFeature() override;

    [[nodiscard]] std::string_view Name() const noexcept override;
    [[nodiscard]] Result<void> AddPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard,
        const RenderFeatureContext& context) override;

private:
    struct PassData;

    DepthPrepassFeatureDesc desc_{};
    MeshManager* meshes_{nullptr};
    MaterialManager* materials_{nullptr};
    MaterialPipelineResolver* material_pipelines_{nullptr};
    PipelineManager* pipelines_{nullptr};
    GpuResourceManager* resources_{nullptr};
    StandardDrawBindings* bindings_{nullptr};
    std::shared_ptr<PassData> active_frame_{};
};

} // namespace woki::gfx
