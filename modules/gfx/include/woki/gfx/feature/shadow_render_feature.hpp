#pragma once

#include "../draw/standard_draw_bindings.hpp"
#include "../graph/render_feature.hpp"
#include "../lighting/shadow.hpp"

namespace woki::gfx {

struct ShadowRenderFeatureDesc final {
    RenderView light_view{};
    rhi::TextureFormat format{rhi::TextureFormat::Depth32Float};
    u32 resolution{2048};
    f32 clear_depth{1.0F};
    f32 depth_bias{0.001F};
    f32 normal_bias{0.002F};
    f32 strength{1.0F};
    u32 light_index{0};
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
