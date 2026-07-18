#pragma once

#include "resource_id.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include <woki/rhi.hpp>

namespace woki::gfx {

enum class ResourceLifetime : u8 {
    Persistent = 0,
    Dynamic,
    Transient,
};

struct BufferResourceDesc final {
    AssetId asset_id{};
    rhi::BufferDesc gpu{};
    std::vector<std::byte> initial_data{};
    ResourceLifetime lifetime{ResourceLifetime::Persistent};
    bool retain_cpu_copy{false};
};

struct TextureSubresourceData final {
    std::vector<std::byte> data{};
    u32 mip_level{0};
    u32 array_layer{0};
    u32 bytes_per_row{0};
    u32 rows_per_image{0};
};

struct TextureResourceDesc final {
    AssetId asset_id{};
    rhi::TextureDesc gpu{};
    rhi::TextureViewDesc default_view{};
    std::vector<TextureSubresourceData> initial_data{};
    ResourceLifetime lifetime{ResourceLifetime::Persistent};
    bool retain_cpu_copy{false};
};

struct SamplerResourceDesc final {
    AssetId asset_id{};
    rhi::SamplerDesc gpu{};
};

[[nodiscard]] Result<void> Validate(const BufferResourceDesc& desc);
[[nodiscard]] Result<void> Validate(const TextureResourceDesc& desc);
[[nodiscard]] Result<void> Validate(const SamplerResourceDesc& desc);

} // namespace woki::gfx
