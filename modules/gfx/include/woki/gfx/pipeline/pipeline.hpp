#pragma once

#include "../resource/resource_id.hpp"
#include "../resource/resource_types.hpp"

#include <optional>
#include <string>
#include <vector>

#include <woki/rhi.hpp>

namespace woki::gfx {

struct VertexAttribute final {
    rhi::VertexFormat format{rhi::VertexFormat::Float32x3};
    u64 offset{0};
    u32 shader_location{0};
};

struct VertexBufferLayout final {
    u64 stride{0};
    rhi::VertexStepMode step_mode{rhi::VertexStepMode::Vertex};
    std::vector<VertexAttribute> attributes{};
};

struct ColorTarget final {
    rhi::TextureFormat format{rhi::TextureFormat::Undefined};
    std::optional<rhi::BlendStateDesc> blend{};
    rhi::ColorWriteMask write_mask{rhi::ColorWriteMask::All};
};

struct GraphicsPipelineDesc final {
    AssetId asset_id{};
    std::string label{};
    ShaderHandle shader{};
    std::vector<VertexBufferLayout> vertex_buffers{};
    std::vector<ColorTarget> color_targets{};
    rhi::PrimitiveStateDesc primitive{};
    std::optional<rhi::DepthStencilStateDesc> depth_stencil{};
};

[[nodiscard]] Result<void> Validate(const GraphicsPipelineDesc& desc);

} // namespace woki::gfx
