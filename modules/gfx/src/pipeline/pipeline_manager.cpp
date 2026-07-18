#include <woki/gfx/pipeline/pipeline_manager.hpp>

#include "../resource/resource_registry.hpp"

#include <woki/gfx/shader/shader_manager.hpp>

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <utility>

namespace woki::gfx {
namespace {

struct PipelineRecord final {
    GraphicsPipelineDesc desc{};
    scope<rhi::RenderPipeline> gpu{};
};

[[nodiscard]] const ShaderSource* FindStage(
    const ShaderDesc& shader, const ShaderStage stage) noexcept {
    const auto iterator = std::ranges::find(shader.sources, stage, &ShaderSource::stage);
    return iterator == shader.sources.end() ? nullptr : &*iterator;
}

} // namespace

Result<void> Validate(const GraphicsPipelineDesc& desc) {
    if (!desc.shader) {
        return Err(ErrorCode::ValidationNullValue, "Graphics pipeline requires a shader");
    }
    if (desc.color_targets.empty() && !desc.depth_stencil.has_value()) {
        return Err(
            ErrorCode::ValidationNullValue, "Graphics pipeline requires a color or depth target");
    }

    std::unordered_set<u32> shader_locations{};
    for (const auto& buffer : desc.vertex_buffers) {
        if (buffer.stride == 0 || buffer.attributes.empty()) {
            return Err(ErrorCode::ValidationOutOfRange,
                "Vertex buffer layouts require a stride and attributes");
        }
        if (buffer.step_mode == rhi::VertexStepMode::Undefined) {
            return Err(ErrorCode::ValidationInvalidState, "Vertex step mode must be specified");
        }
        for (const auto& attribute : buffer.attributes) {
            if (attribute.offset >= buffer.stride) {
                return Err(ErrorCode::ValidationOutOfRange, "Vertex attribute exceeds its stride");
            }
            if (!shader_locations.insert(attribute.shader_location).second) {
                return Err(
                    ErrorCode::ValidationInvalidState, "Vertex shader locations must be unique");
            }
        }
    }
    for (const auto& target : desc.color_targets) {
        if (target.format == rhi::TextureFormat::Undefined) {
            return Err(ErrorCode::GraphicsInvalidFormat, "Color target format must be specified");
        }
    }
    if (desc.depth_stencil && desc.depth_stencil->format == rhi::TextureFormat::Undefined) {
        return Err(ErrorCode::GraphicsInvalidFormat, "Depth target format must be specified");
    }
    if (desc.depth_fragment && (!desc.depth_stencil || !desc.color_targets.empty())) {
        return Err(ErrorCode::ValidationInvalidState,
            "Depth fragment execution requires a depth-only pipeline");
    }
    return Ok();
}

class PipelineManager::Impl final {
public:
    Impl(rhi::Device& device, ShaderManager& shaders) : device_(&device), shaders_(&shaders) {}

    [[nodiscard]] Result<scope<rhi::RenderPipeline>> Compile(
        const GraphicsPipelineDesc& desc) const {
        const ShaderDesc* shader_desc = shaders_->Description(desc.shader);
        auto* vertex_module = shaders_->Resolve(desc.shader, ShaderStage::Vertex);
        auto* fragment_module = shaders_->Resolve(desc.shader, ShaderStage::Fragment);
        const bool needs_fragment = !desc.color_targets.empty() || desc.depth_fragment;
        if (shader_desc == nullptr || vertex_module == nullptr ||
            (needs_fragment && fragment_module == nullptr)) {
            return Err(ErrorCode::FailedToAcquireResource,
                "Graphics pipeline is missing a required shader stage");
        }

        const ShaderSource* vertex_source = FindStage(*shader_desc, ShaderStage::Vertex);
        const ShaderSource* fragment_source = FindStage(*shader_desc, ShaderStage::Fragment);
        WOKI_ASSERT(vertex_source != nullptr);

        std::vector<std::vector<rhi::VertexAttributeDesc>> attribute_storage{};
        attribute_storage.reserve(desc.vertex_buffers.size());
        for (const auto& buffer : desc.vertex_buffers) {
            auto& attributes = attribute_storage.emplace_back();
            attributes.reserve(buffer.attributes.size());
            for (const auto& attribute : buffer.attributes) {
                attributes.push_back({
                    .format = attribute.format,
                    .offset = attribute.offset,
                    .shader_location = attribute.shader_location,
                });
            }
        }

        std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers{};
        vertex_buffers.reserve(desc.vertex_buffers.size());
        for (std::size_t index = 0; index < desc.vertex_buffers.size(); ++index) {
            vertex_buffers.push_back({
                .step_mode = desc.vertex_buffers[index].step_mode,
                .array_stride = desc.vertex_buffers[index].stride,
                .attributes = attribute_storage[index],
            });
        }

        std::vector<rhi::ColorTargetStateDesc> color_targets{};
        color_targets.reserve(desc.color_targets.size());
        for (const auto& target : desc.color_targets) {
            color_targets.push_back({
                .format = target.format,
                .blend = target.blend ? &*target.blend : nullptr,
                .write_mask = target.write_mask,
            });
        }

        const rhi::VertexStateDesc vertex{
            .module = vertex_module,
            .entry_point = vertex_source->entry_point,
            .buffers = vertex_buffers,
        };
        std::optional<rhi::FragmentStateDesc> fragment{};
        if (needs_fragment) {
            WOKI_ASSERT(fragment_source != nullptr);
            fragment = rhi::FragmentStateDesc{
                .module = fragment_module,
                .entry_point = fragment_source->entry_point,
                .targets = color_targets,
            };
        }
        const rhi::RenderPipelineDescTyped native{
            .vertex = &vertex,
            .primitive = &desc.primitive,
            .depth_stencil = desc.depth_stencil ? &*desc.depth_stencil : nullptr,
            .fragment = fragment ? &*fragment : nullptr,
            .label = desc.label,
        };
        return device_->CreateRenderPipeline(native);
    }

    rhi::Device* device_{nullptr};
    ShaderManager* shaders_{nullptr};
    detail::ResourceRegistry<PipelineRecord, PipelineTag> pipelines{};

    struct RetiredPipeline final {
        u64 after_submission{0};
        scope<rhi::RenderPipeline> gpu{};
    };
    std::vector<RetiredPipeline> retired{};
};

PipelineManager::PipelineManager(rhi::Device& device, ShaderManager& shaders)
    : impl_(std::make_unique<Impl>(device, shaders)) {}

PipelineManager::~PipelineManager() { Clear(); }

Result<PipelineHandle> PipelineManager::Create(const GraphicsPipelineDesc& desc) {
    if (auto validation = Validate(desc); !validation) {
        return Err(validation.error());
    }
    if (const PipelineHandle existing = Find(desc.asset_id); existing) {
        return Ok(existing);
    }
    auto gpu = impl_->Compile(desc);
    if (!gpu) {
        return Err(gpu.error());
    }
    return Ok(impl_->pipelines.Create(desc.asset_id, desc.label, ResourceState::Resident,
        PipelineRecord{.desc = desc, .gpu = std::move(*gpu)}));
}

Result<void> PipelineManager::Rebuild(
    const PipelineHandle pipeline, const u64 retire_after_submission) {
    auto* entry = impl_->pipelines.TryGet(pipeline);
    if (entry == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Pipeline handle is not active");
    }
    auto replacement = impl_->Compile(entry->value.desc);
    if (!replacement) {
        return Err(replacement.error());
    }
    impl_->retired.push_back({retire_after_submission, std::move(entry->value.gpu)});
    entry->value.gpu = std::move(*replacement);
    entry->metadata.version = entry->metadata.version.Next();
    entry->metadata.state = ResourceState::Resident;
    return Ok();
}

Result<void> PipelineManager::RebuildForShader(
    const ShaderHandle shader, const u64 retire_after_submission) {
    struct Replacement final {
        PipelineHandle handle{};
        scope<rhi::RenderPipeline> gpu{};
    };
    std::vector<Replacement> replacements{};
    impl_->pipelines.Each([&](const PipelineHandle handle, const auto& entry) {
        if (entry.value.desc.shader == shader) {
            replacements.push_back({.handle = handle});
        }
    });

    for (auto& replacement : replacements) {
        const auto* record = impl_->pipelines.TryGetValue(replacement.handle);
        WOKI_ASSERT(record != nullptr);
        auto gpu = impl_->Compile(record->desc);
        if (!gpu) {
            return Err(gpu.error());
        }
        replacement.gpu = std::move(*gpu);
    }

    for (auto& replacement : replacements) {
        auto* entry = impl_->pipelines.TryGet(replacement.handle);
        WOKI_ASSERT(entry != nullptr);
        impl_->retired.push_back({retire_after_submission, std::move(entry->value.gpu)});
        entry->value.gpu = std::move(replacement.gpu);
        entry->metadata.version = entry->metadata.version.Next();
        entry->metadata.state = ResourceState::Resident;
    }
    return Ok();
}

PipelineHandle PipelineManager::Find(const AssetId asset_id) const noexcept {
    return impl_->pipelines.Find(asset_id);
}

rhi::RenderPipeline* PipelineManager::Resolve(const PipelineHandle pipeline) noexcept {
    auto* record = impl_->pipelines.TryGetValue(pipeline);
    return record == nullptr ? nullptr : record->gpu.get();
}

const rhi::RenderPipeline* PipelineManager::Resolve(const PipelineHandle pipeline) const noexcept {
    const auto* record = impl_->pipelines.TryGetValue(pipeline);
    return record == nullptr ? nullptr : record->gpu.get();
}

const ResourceMetadata* PipelineManager::Metadata(const PipelineHandle pipeline) const noexcept {
    const auto* entry = impl_->pipelines.TryGet(pipeline);
    return entry == nullptr ? nullptr : &entry->metadata;
}

const GraphicsPipelineDesc* PipelineManager::Description(
    const PipelineHandle pipeline) const noexcept {
    const auto* record = impl_->pipelines.TryGetValue(pipeline);
    return record == nullptr ? nullptr : &record->desc;
}

bool PipelineManager::Destroy(const PipelineHandle pipeline) {
    return impl_->pipelines.Remove(pipeline);
}

bool PipelineManager::Retire(const PipelineHandle pipeline, const u64 after_submission) {
    auto* record = impl_->pipelines.TryGetValue(pipeline);
    if (record == nullptr) {
        return false;
    }
    impl_->retired.push_back({after_submission, std::move(record->gpu)});
    return impl_->pipelines.Remove(pipeline);
}

void PipelineManager::Collect(const u64 completed_submission) {
    std::erase_if(impl_->retired, [completed_submission](const auto& retired) {
        return retired.after_submission <= completed_submission;
    });
}

std::size_t PipelineManager::Size() const noexcept { return impl_->pipelines.Size(); }

std::size_t PipelineManager::RetiredCount() const noexcept { return impl_->retired.size(); }

void PipelineManager::Clear() {
    impl_->pipelines.Clear();
    Collect(std::numeric_limits<u64>::max());
}

} // namespace woki::gfx
