#include <woki/gfx/pipeline/material_pipeline_resolver.hpp>

#include <algorithm>

namespace woki::gfx {

MaterialPipelineKey MakeMaterialPipelineKey(const MaterialDesc& material,
    const RenderPassClass pass, const RenderTargetSignature& targets,
    const StringId vertex_layout) {
    return {
        .shader = material.shader,
        .model = material.model,
        .blend_mode = material.blend_mode,
        .pass = pass,
        .targets = targets,
        .vertex_layout = vertex_layout,
        .double_sided = material.double_sided,
        .depth_write = material.depth_write,
    };
}

Result<void> Validate(const MaterialPipelineKey& key) {
    if (!key.shader) {
        return Err(ErrorCode::ValidationNullValue, "Material pipeline key requires a shader");
    }
    if (key.vertex_layout.Empty()) {
        return Err(
            ErrorCode::ValidationNullValue, "Material pipeline key requires a vertex layout");
    }
    if (key.targets.sample_count == 0) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Material pipeline target sample count must be nonzero");
    }
    if (key.pass != RenderPassClass::DepthOnly && key.targets.color_formats.empty()) {
        return Err(ErrorCode::ValidationNullValue,
            "Color render passes require at least one target format");
    }
    if (key.pass == RenderPassClass::DepthOnly && !key.targets.depth_format) {
        return Err(ErrorCode::ValidationNullValue,
            "Depth-only render pass requires a depth target format");
    }
    for (const auto format : key.targets.color_formats) {
        if (format == rhi::TextureFormat::Undefined) {
            return Err(ErrorCode::GraphicsInvalidFormat,
                "Material pipeline color target format is undefined");
        }
    }
    if (key.targets.depth_format == rhi::TextureFormat::Undefined) {
        return Err(ErrorCode::GraphicsInvalidFormat, "Material pipeline depth format is undefined");
    }
    if (key.pass == RenderPassClass::ForwardTransparent &&
        key.blend_mode != MaterialBlendMode::Translucent) {
        return Err(ErrorCode::ValidationInvalidState,
            "Transparent render pass requires a translucent material");
    }
    if (key.pass == RenderPassClass::ForwardOpaque &&
        key.blend_mode == MaterialBlendMode::Translucent) {
        return Err(ErrorCode::ValidationInvalidState,
            "Opaque render pass cannot use a translucent material");
    }
    return Ok();
}

auto MaterialPipelineResolver::Find(const MaterialPipelineKey& key) noexcept {
    return std::ranges::find(entries_, key, &Entry::key);
}

auto MaterialPipelineResolver::Find(const MaterialPipelineKey& key) const noexcept {
    return std::ranges::find(entries_, key, &Entry::key);
}

Result<void> MaterialPipelineResolver::Register(
    const MaterialPipelineKey& key, const PipelineHandle pipeline) {
    if (auto validation = Validate(key); !validation) {
        return validation;
    }
    if (!pipeline) {
        return Err(ErrorCode::ValidationNullValue, "Material pipeline handle is invalid");
    }
    if (Find(key) != entries_.end()) {
        return Err(
            ErrorCode::ValidationInvalidState, "Material pipeline variant is already registered");
    }
    entries_.push_back({.key = key, .pipeline = pipeline});
    return Ok();
}

Result<void> MaterialPipelineResolver::Replace(
    const MaterialPipelineKey& key, const PipelineHandle pipeline) {
    if (auto validation = Validate(key); !validation) {
        return validation;
    }
    if (!pipeline) {
        return Err(ErrorCode::ValidationNullValue, "Material pipeline handle is invalid");
    }
    const auto iterator = Find(key);
    if (iterator == entries_.end()) {
        return Err(
            ErrorCode::FailedToAcquireResource, "Material pipeline variant is not registered");
    }
    iterator->pipeline = pipeline;
    return Ok();
}

PipelineHandle MaterialPipelineResolver::Resolve(const MaterialPipelineKey& key) const noexcept {
    const auto iterator = Find(key);
    return iterator != entries_.end() ? iterator->pipeline : PipelineHandle{};
}

PipelineHandle MaterialPipelineResolver::Resolve(const MaterialDesc& material,
    const RenderPassClass pass, const RenderTargetSignature& targets,
    const StringId vertex_layout) const noexcept {
    return Resolve(MakeMaterialPipelineKey(material, pass, targets, vertex_layout));
}

bool MaterialPipelineResolver::Remove(const MaterialPipelineKey& key) {
    const auto iterator = Find(key);
    if (iterator == entries_.end()) {
        return false;
    }
    entries_.erase(iterator);
    return true;
}

std::size_t MaterialPipelineResolver::RemovePipeline(const PipelineHandle pipeline) {
    return std::erase_if(
        entries_, [pipeline](const Entry& entry) { return entry.pipeline == pipeline; });
}

bool MaterialPipelineResolver::Contains(const MaterialPipelineKey& key) const noexcept {
    return Find(key) != entries_.end();
}

std::size_t MaterialPipelineResolver::Size() const noexcept { return entries_.size(); }
void MaterialPipelineResolver::Clear() noexcept { entries_.clear(); }

} // namespace woki::gfx
