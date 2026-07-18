#pragma once

#include "../draw/standard_draw_bindings.hpp"
#include "../graph/render_feature.hpp"
#include "../lighting/shadow.hpp"

#include <vector>

namespace woki::gfx {

struct CascadedShadow final {
    RenderView light_view{};
    f32 far_distance{0.0F};
};

struct CascadedShadowFeatureDesc final {
    std::vector<CascadedShadow> cascades{};
    rhi::TextureFormat format{rhi::TextureFormat::Depth32Float};
    u32 cascade_resolution{2048};
    f32 clear_depth{1.0F};
    f32 depth_bias{0.001F};
    f32 normal_bias{0.002F};
    f32 strength{1.0F};
    u32 light_index{0};
};

[[nodiscard]] Result<void> Validate(const CascadedShadowFeatureDesc& desc);

class CascadedShadowFeature final : public RenderFeature {
public:
    CascadedShadowFeature(const CascadedShadowFeatureDesc& desc, MeshManager& meshes,
        MaterialManager& materials, MaterialPipelineResolver& material_pipelines,
        PipelineManager& pipelines, GpuResourceManager& resources, StandardDrawBindings& bindings);
    ~CascadedShadowFeature() override;

    [[nodiscard]] std::string_view Name() const noexcept override;
    [[nodiscard]] Result<void> AddPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard,
        const RenderFeatureContext& context) override;

private:
    struct PassData;

    CascadedShadowFeatureDesc desc_{};
    MeshManager* meshes_{nullptr};
    MaterialManager* materials_{nullptr};
    MaterialPipelineResolver* material_pipelines_{nullptr};
    PipelineManager* pipelines_{nullptr};
    GpuResourceManager* resources_{nullptr};
    StandardDrawBindings* bindings_{nullptr};
    std::shared_ptr<PassData> active_frame_{};
};

} // namespace woki::gfx
