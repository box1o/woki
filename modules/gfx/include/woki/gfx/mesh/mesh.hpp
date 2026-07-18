#pragma once

#include "../resource/resource_id.hpp"
#include "../resource/resource_types.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include <woki/rhi.hpp>

namespace woki::gfx {

struct VertexStreamDesc final {
    u64 stride{0};
    u32 vertex_count{0};
    std::vector<std::byte> data{};
};

struct SubmeshDesc final {
    u32 first_index{0};
    u32 index_count{0};
    i32 base_vertex{0};
    u32 material_slot{0};
};

struct MeshDesc final {
    AssetId asset_id{};
    std::string label{};
    StringId vertex_layout{};
    std::vector<VertexStreamDesc> vertex_streams{};
    rhi::IndexFormat index_format{rhi::IndexFormat::Uint32};
    u32 index_count{0};
    std::vector<std::byte> index_data{};
    std::vector<SubmeshDesc> submeshes{};
    bool retain_cpu_copy{false};
};

struct MeshView final {
    StringId vertex_layout{};
    std::vector<BufferHandle> vertex_buffers{};
    BufferHandle index_buffer{};
    rhi::IndexFormat index_format{rhi::IndexFormat::Undefined};
    u32 vertex_count{0};
    u32 index_count{0};
    std::vector<SubmeshDesc> submeshes{};
};

[[nodiscard]] Result<void> Validate(const MeshDesc& desc);

} // namespace woki::gfx
