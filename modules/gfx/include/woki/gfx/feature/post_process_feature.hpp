#pragma once

#include "../graph/render_feature.hpp"
#include "../graph/render_outputs.hpp"
#include "../pipeline/pipeline_manager.hpp"
#include "../resource/gpu_resource_manager.hpp"

namespace woki::gfx {

struct PostProcessFeatureDesc final {
    PipelineHandle pipeline{};
    SamplerHandle sampler{};
    u32 bind_group{0};
    u32 texture_binding{0};
    u32 sampler_binding{1};
};

[[nodiscard]] Result<void> Validate(const PostProcessFeatureDesc& desc);

class PostProcessFeature final : public RenderFeature {
public:
    PostProcessFeature(const PostProcessFeatureDesc& desc, PipelineManager& pipelines,
        GpuResourceManager& resources);
    ~PostProcessFeature() override;

    [[nodiscard]] std::string_view Name() const noexcept override;
    [[nodiscard]] Result<void> AddPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard,
        const RenderFeatureContext& context) override;

private:
    PostProcessFeatureDesc desc_{};
    PipelineManager* pipelines_{nullptr};
    GpuResourceManager* resources_{nullptr};
};

} // namespace woki::gfx
