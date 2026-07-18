#pragma once

#include "../draw/standard_draw_bindings.hpp"
#include "../graph/render_feature.hpp"

namespace woki::gfx {

namespace render_outputs {
inline const StringId kShadowDepth{"render.shadow_depth"};
} // namespace render_outputs

struct ShadowRenderFeatureDesc final {
    RenderView light_view{};
    rhi::TextureFormat format{rhi::TextureFormat::Depth32Float};
    u32 resolution{2048};
    f32 clear_depth{1.0F};
};

[[nodiscard]] Result<void> Validate(const ShadowRenderFeatureDesc& desc);

class ShadowRenderFeature final : public RenderFeature {
public:
    ShadowRenderFeature(const ShadowRenderFeatureDesc& desc, MeshManager& meshes,
        MaterialManager& materials, MaterialPipelineResolver& material_pipelines,
        PipelineManager& pipelines, GpuResourceManager& resources, StandardDrawBindings& bindings);
    ~ShadowRenderFeature() override;

    [[nodiscard]] std::string_view Name() const noexcept override;
    [[nodiscard]] Result<void> AddPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard,
        const RenderFeatureContext& context) override;

private:
    struct PassData;

    ShadowRenderFeatureDesc desc_{};
    MeshManager* meshes_{nullptr};
    MaterialManager* materials_{nullptr};
    MaterialPipelineResolver* material_pipelines_{nullptr};
    PipelineManager* pipelines_{nullptr};
    GpuResourceManager* resources_{nullptr};
    StandardDrawBindings* bindings_{nullptr};
    std::shared_ptr<PassData> active_frame_{};
};

} // namespace woki::gfx
