#pragma once

#include "../draw/standard_draw_bindings.hpp"
#include "../graph/render_feature.hpp"
#include "../graph/render_outputs.hpp"
#include "../lighting/light.hpp"
#include "../lighting/shadow.hpp"

namespace woki::gfx {

struct ForwardRenderFeatureDesc final {
    RenderTargetSignature targets{};
    rhi::Color clear_color{0.02, 0.02, 0.025, 1.0};
    f32 clear_depth{1.0F};
    math::vec3f ambient_light{0.02F};
    u32 maximum_lights{256};
    bool offscreen_color{false};
    std::optional<EnvironmentLighting> environment{};
};

class ForwardRenderFeature final : public RenderFeature {
public:
    ForwardRenderFeature(const ForwardRenderFeatureDesc& desc, MeshManager& meshes,
        MaterialManager& materials, MaterialPipelineResolver& material_pipelines,
        PipelineManager& pipelines, GpuResourceManager& resources, StandardDrawBindings& bindings);
    ~ForwardRenderFeature() override;

    [[nodiscard]] std::string_view Name() const noexcept override;
    [[nodiscard]] Result<void> AddPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard,
        const RenderFeatureContext& context) override;

private:
    struct PassData;

    ForwardRenderFeatureDesc desc_{};
    MeshManager* meshes_{nullptr};
    MaterialManager* materials_{nullptr};
    MaterialPipelineResolver* material_pipelines_{nullptr};
    PipelineManager* pipelines_{nullptr};
    GpuResourceManager* resources_{nullptr};
    StandardDrawBindings* bindings_{nullptr};
    std::shared_ptr<PassData> active_frame_{};
};

} // namespace woki::gfx
