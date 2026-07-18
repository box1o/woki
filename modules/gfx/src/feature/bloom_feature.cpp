#include <woki/gfx/feature/bloom_feature.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace woki::gfx {
namespace {

struct alignas(16) ThresholdParameters final {
    f32 threshold{1.0F};
    f32 soft_knee{0.5F};
    f32 padding[2]{};
};

struct alignas(16) BlurParameters final {
    math::vec2f direction{};
    f32 padding[2]{};
};

struct alignas(16) CompositeParameters final {
    f32 intensity{0.08F};
    f32 padding[3]{};
};

static_assert(sizeof(ThresholdParameters) == 16);
static_assert(sizeof(BlurParameters) == 16);
static_assert(sizeof(CompositeParameters) == 16);

[[nodiscard]] rhi::ExtentMode ScaleExtent(
    const rhi::ExtentMode& extent, const f32 scale) noexcept {
    switch (extent.kind) {
    case rhi::ExtentModeKind::Swapchain:
        return rhi::ExtentMode::Relative(scale, scale);
    case rhi::ExtentModeKind::Fixed:
        return rhi::ExtentMode::Fixed(
            std::max(1U, static_cast<u32>(static_cast<f32>(extent.width) * scale)),
            std::max(1U, static_cast<u32>(static_cast<f32>(extent.height) * scale)));
    case rhi::ExtentModeKind::Relative:
        return rhi::ExtentMode::Relative(
            extent.relative_width * scale, extent.relative_height * scale);
    }
    return rhi::ExtentMode::Relative(scale, scale);
}

template <typename T>
[[nodiscard]] Result<UniformBufferSlice> WriteParameters(
    FrameUniformBuffer& uniforms, const T& value) {
    return uniforms.Write(std::as_bytes(std::span{&value, 1}));
}

} // namespace

Result<void> Validate(const BloomFeatureDesc& desc) {
    if (!desc.threshold_pipeline || !desc.blur_pipeline || !desc.composite_pipeline ||
        !desc.sampler) {
        return Err(ErrorCode::ValidationNullValue,
            "Bloom requires threshold, blur, composite pipelines, and a sampler");
    }
    if (!std::isfinite(desc.resolution_scale) || desc.resolution_scale <= 0.0F ||
        desc.resolution_scale > 1.0F) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Bloom resolution scale must be finite and in (0, 1]");
    }
    if (!std::isfinite(desc.threshold) || desc.threshold < 0.0F ||
        !std::isfinite(desc.soft_knee) || desc.soft_knee < 0.0F ||
        !std::isfinite(desc.intensity) || desc.intensity < 0.0F) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Bloom threshold, knee, and intensity must be finite and nonnegative");
    }
    if (desc.blur_iterations == 0 || desc.blur_iterations > 8) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Bloom blur iterations must be in [1, 8]");
    }
    return Ok();
}

struct BloomFeature::PassData final {
    std::vector<UniformBufferSlice> parameters{};
};

BloomFeature::BloomFeature(const BloomFeatureDesc& desc, PipelineManager& pipelines,
    GpuResourceManager& resources, FrameUniformBuffer& uniforms)
    : desc_(desc), pipelines_(&pipelines), resources_(&resources), uniforms_(&uniforms) {}

BloomFeature::~BloomFeature() = default;

std::string_view BloomFeature::Name() const noexcept { return "Bloom"; }

Result<void> BloomFeature::EncodeSingleInput(rhi::RenderPassContext& context,
    const PipelineHandle pipeline_handle, const u32 parameter_index) const {
    const auto frame = active_frame_;
    if (!frame || parameter_index >= frame->parameters.size()) {
        return Err(ErrorCode::InvalidState, "Bloom pass has no active parameter data");
    }
    const rhi::RenderPipeline* pipeline = pipelines_->Resolve(pipeline_handle);
    rhi::Sampler* sampler = resources_->Resolve(desc_.sampler);
    const UniformBufferSlice& parameters = frame->parameters[parameter_index];
    rhi::Buffer* parameter_buffer = resources_->Resolve(parameters.buffer);
    if (pipeline == nullptr || sampler == nullptr || parameter_buffer == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource,
            "Bloom pass pipeline, sampler, or parameter buffer is unavailable");
    }
    auto layout = pipeline->GetBindGroupLayout(bloom_bindings::kGroup);
    if (!layout) {
        return Err(ErrorCode::FailedToAcquireResource,
            "Bloom pass bind group layout is unavailable");
    }
    const std::array<rhi::BindGroupEntryDesc, 3> entries{{
        {.binding = bloom_bindings::kSourceTexture, .texture_view = &context.sample(0)},
        {.binding = bloom_bindings::kSingleInputSampler, .sampler = sampler},
        {.binding = bloom_bindings::kSingleInputParameters,
            .buffer = parameter_buffer,
            .offset = parameters.offset,
            .size = parameters.size},
    }};
    auto group = context.device().CreateBindGroup(
        {.layout = layout.get(), .entries = entries, .label = "Bloom inputs"});
    if (!group) {
        return Err(group.error());
    }
    context.encoder().SetPipeline(*pipeline);
    context.encoder().SetBindGroup(bloom_bindings::kGroup, group->get());
    context.encoder().Draw(3);
    return Ok();
}

Result<void> BloomFeature::EncodeComposite(
    rhi::RenderPassContext& context, const u32 parameter_index) const {
    const auto frame = active_frame_;
    if (!frame || parameter_index >= frame->parameters.size()) {
        return Err(ErrorCode::InvalidState, "Bloom composite has no active parameter data");
    }
    const rhi::RenderPipeline* pipeline = pipelines_->Resolve(desc_.composite_pipeline);
    rhi::Sampler* sampler = resources_->Resolve(desc_.sampler);
    const UniformBufferSlice& parameters = frame->parameters[parameter_index];
    rhi::Buffer* parameter_buffer = resources_->Resolve(parameters.buffer);
    if (pipeline == nullptr || sampler == nullptr || parameter_buffer == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource,
            "Bloom composite pipeline, sampler, or parameter buffer is unavailable");
    }
    auto layout = pipeline->GetBindGroupLayout(bloom_bindings::kGroup);
    if (!layout) {
        return Err(ErrorCode::FailedToAcquireResource,
            "Bloom composite bind group layout is unavailable");
    }
    const std::array<rhi::BindGroupEntryDesc, 4> entries{{
        {.binding = bloom_bindings::kSourceTexture, .texture_view = &context.sample(0)},
        {.binding = bloom_bindings::kBloomTexture, .texture_view = &context.sample(1)},
        {.binding = bloom_bindings::kCompositeSampler, .sampler = sampler},
        {.binding = bloom_bindings::kCompositeParameters,
            .buffer = parameter_buffer,
            .offset = parameters.offset,
            .size = parameters.size},
    }};
    auto group = context.device().CreateBindGroup(
        {.layout = layout.get(), .entries = entries, .label = "Bloom composite inputs"});
    if (!group) {
        return Err(group.error());
    }
    context.encoder().SetPipeline(*pipeline);
    context.encoder().SetBindGroup(bloom_bindings::kGroup, group->get());
    context.encoder().Draw(3);
    return Ok();
}

Result<void> BloomFeature::AddPasses(
    RenderGraph& graph, RenderGraphBlackboard& blackboard, const RenderFeatureContext&) {
    if (auto validation = Validate(desc_); !validation) {
        return validation;
    }
    const GraphResource source = blackboard.Find(render_outputs::kColor);
    const GraphResourceDesc* source_desc = graph.Resource(source);
    if (!source || source_desc == nullptr || source_desc->kind != GraphResourceKind::Texture ||
        source_desc->origin != GraphResourceOrigin::Transient ||
        source_desc->transient.sample_count != 1) {
        return Err(ErrorCode::ValidationInvalidState,
            "Bloom requires a single-sampled transient HDR color input");
    }
    if (source_desc->transient.format != rhi::TextureFormat::RGBA16Float &&
        source_desc->transient.format != rhi::TextureFormat::RG11B10Ufloat) {
        return Err(ErrorCode::GraphicsInvalidFormat,
            "Bloom requires RGBA16Float or RG11B10Ufloat HDR color");
    }
    if (!HasFlag(source_desc->transient.usage, rhi::TextureUsage::TextureBinding)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Bloom HDR input requires TextureBinding usage");
    }

    const rhi::TransientDesc bloom_texture{
        .format = source_desc->transient.format,
        .usage = rhi::TextureUsage::RenderAttachment | rhi::TextureUsage::TextureBinding,
        .extent = ScaleExtent(source_desc->transient.extent, desc_.resolution_scale),
    };
    auto ping = graph.AddTransientTexture(rhi::TransientDesc{
        .label = "Bloom ping",
        .format = bloom_texture.format,
        .usage = bloom_texture.usage,
        .extent = bloom_texture.extent,
    });
    auto pong = graph.AddTransientTexture(rhi::TransientDesc{
        .label = "Bloom pong",
        .format = bloom_texture.format,
        .usage = bloom_texture.usage,
        .extent = bloom_texture.extent,
    });
    auto output = graph.AddTransientTexture({
        .label = "Bloom composite",
        .format = source_desc->transient.format,
        .usage = rhi::TextureUsage::RenderAttachment | rhi::TextureUsage::TextureBinding,
        .extent = source_desc->transient.extent,
    });
    if (!ping || !pong || !output) {
        return Err(!ping ? ping.error() : !pong ? pong.error() : output.error());
    }

    std::vector<UniformBufferSlice> parameters{};
    parameters.reserve(2 + desc_.blur_iterations * 2);
    auto threshold = WriteParameters(
        *uniforms_, ThresholdParameters{.threshold = desc_.threshold, .soft_knee = desc_.soft_knee});
    if (!threshold) {
        return Err(threshold.error());
    }
    parameters.push_back(*threshold);
    for (u32 iteration = 0; iteration < desc_.blur_iterations; ++iteration) {
        auto horizontal =
            WriteParameters(*uniforms_, BlurParameters{.direction = {1.0F, 0.0F}});
        auto vertical = WriteParameters(*uniforms_, BlurParameters{.direction = {0.0F, 1.0F}});
        if (!horizontal || !vertical) {
            return Err(!horizontal ? horizontal.error() : vertical.error());
        }
        parameters.push_back(*horizontal);
        parameters.push_back(*vertical);
    }
    auto composite =
        WriteParameters(*uniforms_, CompositeParameters{.intensity = desc_.intensity});
    if (!composite) {
        return Err(composite.error());
    }
    parameters.push_back(*composite);
    active_frame_ = std::make_shared<PassData>(PassData{.parameters = std::move(parameters)});

    auto threshold_pass = graph.AddPass({
        .label = "Bloom threshold",
        .resources = {{.resource = source, .access = GraphAccess::Read},
            {.resource = *ping, .access = GraphAccess::Write}},
        .colors = {{.resource = *ping}},
        .samples = {{.resource = source}},
        .execute = [this](rhi::RenderPassContext& context) {
            return EncodeSingleInput(context, desc_.threshold_pipeline, 0);
        },
    });
    if (!threshold_pass) {
        return Err(threshold_pass.error());
    }

    u32 parameter_index = 1;
    for (u32 iteration = 0; iteration < desc_.blur_iterations; ++iteration) {
        auto horizontal = graph.AddPass({
            .label = "Bloom blur horizontal " + std::to_string(iteration),
            .resources = {{.resource = *ping, .access = GraphAccess::Read},
                {.resource = *pong, .access = GraphAccess::Write}},
            .colors = {{.resource = *pong}},
            .samples = {{.resource = *ping}},
            .execute = [this, parameter_index](rhi::RenderPassContext& context) {
                return EncodeSingleInput(context, desc_.blur_pipeline, parameter_index);
            },
        });
        if (!horizontal) {
            return Err(horizontal.error());
        }
        ++parameter_index;
        auto vertical = graph.AddPass({
            .label = "Bloom blur vertical " + std::to_string(iteration),
            .resources = {{.resource = *pong, .access = GraphAccess::Read},
                {.resource = *ping, .access = GraphAccess::Write}},
            .colors = {{.resource = *ping}},
            .samples = {{.resource = *pong}},
            .execute = [this, parameter_index](rhi::RenderPassContext& context) {
                return EncodeSingleInput(context, desc_.blur_pipeline, parameter_index);
            },
        });
        if (!vertical) {
            return Err(vertical.error());
        }
        ++parameter_index;
    }

    auto composite_pass = graph.AddPass({
        .label = "Bloom composite",
        .resources = {{.resource = source, .access = GraphAccess::Read},
            {.resource = *ping, .access = GraphAccess::Read},
            {.resource = *output, .access = GraphAccess::Write}},
        .colors = {{.resource = *output}},
        .samples = {{.resource = source}, {.resource = *ping}},
        .execute = [this, parameter_index](rhi::RenderPassContext& context) {
            return EncodeComposite(context, parameter_index);
        },
    });
    if (!composite_pass) {
        return Err(composite_pass.error());
    }
    return blackboard.Replace(render_outputs::kColor, *output);
}

} // namespace woki::gfx
