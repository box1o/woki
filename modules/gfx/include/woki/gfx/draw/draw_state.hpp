#pragma once

#include "../material/material_manager.hpp"
#include "../mesh/mesh_manager.hpp"
#include "../pipeline/material_pipeline_resolver.hpp"
#include "../pipeline/pipeline_manager.hpp"
#include "../resource/gpu_resource_manager.hpp"
#include "../scene/render_queue.hpp"

#include <vector>

namespace woki::gfx {

struct PreparedDraw final {
    DrawPacket packet{};
    math::mat4f transform{math::mat4f::identity()};
    std::vector<math::mat4f> skin_matrices{};
    PipelineHandle pipeline{};
    MeshView geometry{};
    MaterialDesc material{};
};

struct PreparedDrawBatch final {
    PipelineHandle pipeline{};
    MeshHandle mesh{};
    MaterialHandle material{};
    u32 first_draw{0};
    u32 draw_count{0};
};

struct PreparedDrawList final {
    u64 snapshot_sequence{0};
    RenderPassClass pass{RenderPassClass::ForwardOpaque};
    std::vector<PreparedDraw> draws{};
    std::vector<PreparedDrawBatch> batches{};
};

struct ResolvedDraw final {
    DrawPacket packet{};
    math::mat4f transform{math::mat4f::identity()};
    std::vector<math::mat4f> skin_matrices{};
    const rhi::RenderPipeline* pipeline{nullptr};
    ShaderHandle shader{};
    std::vector<const rhi::Buffer*> vertex_buffers{};
    const rhi::Buffer* index_buffer{nullptr};
    rhi::IndexFormat index_format{rhi::IndexFormat::Undefined};
    MaterialDesc material{};
};

struct ResolvedDrawBatch final {
    const rhi::RenderPipeline* pipeline{nullptr};
    u32 first_draw{0};
    u32 draw_count{0};
};

struct ResolvedDrawList final {
    u64 snapshot_sequence{0};
    RenderPassClass pass{RenderPassClass::ForwardOpaque};
    std::vector<ResolvedDraw> draws{};
    std::vector<ResolvedDrawBatch> batches{};
};

[[nodiscard]] Result<PreparedDrawList> PrepareDraws(const RenderQueue& queue, RenderPassClass pass,
    const RenderTargetSignature& targets, const MeshManager& meshes,
    const MaterialManager& materials, const MaterialPipelineResolver& pipelines);

[[nodiscard]] Result<ResolvedDrawList> ResolveDraws(const PreparedDrawList& prepared,
    const PipelineManager& pipelines, const GpuResourceManager& resources);

} // namespace woki::gfx
