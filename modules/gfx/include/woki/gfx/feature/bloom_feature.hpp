#pragma once

#include "../graph/render_feature.hpp"
#include "../graph/render_outputs.hpp"
#include "../pipeline/pipeline_manager.hpp"
#include "../resource/frame_uniform_buffer.hpp"
#include "../resource/gpu_resource_manager.hpp"

namespace woki::gfx {

namespace bloom_bindings {
inline constexpr u32 kGroup = 0;
inline constexpr u32 kSourceTexture = 0;
inline constexpr u32 kBloomTexture = 1;
inline constexpr u32 kSingleInputSampler = 1;
inline constexpr u32 kSingleInputParameters = 2;
inline constexpr u32 kCompositeSampler = 2;
inline constexpr u32 kCompositeParameters = 3;
} // namespace bloom_bindings

struct BloomFeatureDesc final {
    PipelineHandle threshold_pipeline{};
    PipelineHandle blur_pipeline{};
    PipelineHandle composite_pipeline{};
    SamplerHandle sampler{};
    f32 resolution_scale{0.5F};
    f32 threshold{1.0F};
    f32 soft_knee{0.5F};
    f32 intensity{0.08F};
    u32 blur_iterations{2};
};

[[nodiscard]] Result<void> Validate(const BloomFeatureDesc& desc);

class BloomFeature final : public RenderFeature {
public:
    BloomFeature(const BloomFeatureDesc& desc, PipelineManager& pipelines,
        GpuResourceManager& resources, FrameUniformBuffer& uniforms);
    ~BloomFeature() override;

    [[nodiscard]] std::string_view Name() const noexcept override;
    [[nodiscard]] Result<void> AddPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard,
        const RenderFeatureContext& context) override;

private:
    struct PassData;

    [[nodiscard]] Result<void> EncodeSingleInput(rhi::RenderPassContext& context,
        PipelineHandle pipeline, u32 parameter_index) const;
    [[nodiscard]] Result<void> EncodeComposite(
        rhi::RenderPassContext& context, u32 parameter_index) const;

    BloomFeatureDesc desc_{};
    PipelineManager* pipelines_{nullptr};
    GpuResourceManager* resources_{nullptr};
    FrameUniformBuffer* uniforms_{nullptr};
    std::shared_ptr<PassData> active_frame_{};
};

} // namespace woki::gfx
