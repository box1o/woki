#include <woki/gfx/resource/gpu_resource.hpp>

#include <algorithm>

namespace woki::gfx {

Result<void> Validate(const BufferResourceDesc& desc) {
    if (desc.gpu.size == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "Buffer size must be greater than zero");
    }
    if (desc.gpu.usage == rhi::BufferUsage::None) {
        return Err(ErrorCode::ValidationInvalidState, "Buffer requires at least one usage flag");
    }
    if (desc.initial_data.size() > desc.gpu.size) {
        return Err(ErrorCode::ValidationOutOfRange, "Initial buffer data exceeds buffer size");
    }
    if (desc.lifetime == ResourceLifetime::Transient && desc.retain_cpu_copy) {
        return Err(ErrorCode::ValidationInvalidState, "Transient buffers cannot retain a CPU copy");
    }
    return Ok();
}

Result<void> Validate(const TextureResourceDesc& desc) {
    if (desc.gpu.size.width == 0 || desc.gpu.size.height == 0 ||
        desc.gpu.size.depth_or_array_layers == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "Texture dimensions must be greater than zero");
    }
    if (desc.gpu.mip_level_count == 0 || desc.gpu.sample_count == 0) {
        return Err(
            ErrorCode::ValidationOutOfRange, "Texture mip and sample counts must be nonzero");
    }
    if (desc.gpu.format == rhi::TextureFormat::Undefined) {
        return Err(ErrorCode::GraphicsInvalidFormat, "Texture format must be specified");
    }
    if (desc.gpu.usage == rhi::TextureUsage::None) {
        return Err(ErrorCode::ValidationInvalidState, "Texture requires at least one usage flag");
    }
    if (desc.lifetime == ResourceLifetime::Transient && desc.retain_cpu_copy) {
        return Err(
            ErrorCode::ValidationInvalidState, "Transient textures cannot retain a CPU copy");
    }
    if ((desc.default_view.dimension == rhi::TextureViewDimension::Cube &&
            desc.gpu.size.depth_or_array_layers != 6) ||
        (desc.default_view.dimension == rhi::TextureViewDimension::CubeArray &&
            desc.gpu.size.depth_or_array_layers % 6 != 0)) {
        return Err(
            ErrorCode::ValidationOutOfRange, "Cube texture views require complete six-layer faces");
    }
    if (desc.default_view.format != rhi::TextureFormat::Undefined &&
        desc.default_view.format != desc.gpu.format &&
        std::ranges::find(desc.gpu.view_formats, desc.default_view.format) ==
            desc.gpu.view_formats.end()) {
        return Err(ErrorCode::GraphicsInvalidFormat,
            "Texture default view format is not compatible with the texture");
    }

    for (const auto& subresource : desc.initial_data) {
        if (subresource.data.empty()) {
            return Err(ErrorCode::ValidationNullValue, "Texture subresource data cannot be empty");
        }
        if (subresource.mip_level >= desc.gpu.mip_level_count ||
            subresource.array_layer >= desc.gpu.size.depth_or_array_layers) {
            return Err(
                ErrorCode::ValidationOutOfRange, "Texture subresource is outside the texture");
        }
        if (subresource.bytes_per_row == 0 || subresource.rows_per_image == 0) {
            return Err(ErrorCode::ValidationOutOfRange,
                "Texture subresource layout must specify row strides");
        }
        const u64 required_size = static_cast<u64>(subresource.bytes_per_row) *
                                  static_cast<u64>(subresource.rows_per_image);
        if (required_size > subresource.data.size()) {
            return Err(ErrorCode::ValidationOutOfRange,
                "Texture subresource data is smaller than its declared layout");
        }
    }
    return Ok();
}

Result<void> Validate(const SamplerResourceDesc& desc) {
    if (desc.gpu.lod_min_clamp > desc.gpu.lod_max_clamp) {
        return Err(ErrorCode::ValidationOutOfRange, "Sampler minimum LOD exceeds maximum LOD");
    }
    if (desc.gpu.max_anisotropy == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "Sampler anisotropy must be at least one");
    }
    return Ok();
}

} // namespace woki::gfx
