#include <woki/gfx/draw/draw_state.hpp>

#include <limits>

namespace woki::gfx {

Result<PreparedDrawList> PrepareDraws(const RenderQueue& queue, const RenderPassClass pass,
    const RenderTargetSignature& targets, const MeshManager& meshes,
    const MaterialManager& materials, const MaterialPipelineResolver& pipelines) {
    if (queue.draws.size() > std::numeric_limits<u32>::max()) {
        return Err(ErrorCode::ValidationOutOfRange, "Prepared draw list exceeds its index range");
    }

    PreparedDrawList prepared{.snapshot_sequence = queue.snapshot_sequence};
    prepared.draws.reserve(queue.draws.size());
    prepared.batches.reserve(queue.batches.size());
    for (const auto& queued : queue.draws) {
        auto geometry = meshes.GetView(queued.packet.mesh);
        if (!geometry) {
            return Err(geometry.error());
        }
        auto material = materials.Snapshot(queued.packet.material);
        if (!material) {
            return Err(material.error());
        }
        const PipelineHandle pipeline =
            pipelines.Resolve(*material, pass, targets, geometry->vertex_layout);
        if (!pipeline) {
            return Err(
                ErrorCode::FailedToAcquireResource, "No pipeline variant matches the queued draw");
        }

        const u32 draw_index = static_cast<u32>(prepared.draws.size());
        prepared.draws.push_back({
            .packet = queued.packet,
            .transform = queued.transform,
            .skin_matrices = queued.skin_matrices,
            .pipeline = pipeline,
            .geometry = std::move(*geometry),
            .material = std::move(*material),
        });
        if (prepared.batches.empty() || prepared.batches.back().pipeline != pipeline ||
            prepared.batches.back().mesh != queued.packet.mesh ||
            prepared.batches.back().material != queued.packet.material) {
            prepared.batches.push_back({
                .pipeline = pipeline,
                .mesh = queued.packet.mesh,
                .material = queued.packet.material,
                .first_draw = draw_index,
                .draw_count = 1,
            });
        } else {
            ++prepared.batches.back().draw_count;
        }
    }
    return Ok(std::move(prepared));
}

Result<ResolvedDrawList> ResolveDraws(const PreparedDrawList& prepared,
    const PipelineManager& pipelines, const GpuResourceManager& resources) {
    ResolvedDrawList resolved{.snapshot_sequence = prepared.snapshot_sequence};
    resolved.draws.reserve(prepared.draws.size());
    resolved.batches.reserve(prepared.batches.size());

    for (const auto& draw : prepared.draws) {
        const rhi::RenderPipeline* pipeline = pipelines.Resolve(draw.pipeline);
        if (pipeline == nullptr) {
            return Err(
                ErrorCode::FailedToAcquireResource, "Prepared draw pipeline is no longer active");
        }
        const rhi::Buffer* index_buffer = resources.Resolve(draw.geometry.index_buffer);
        if (index_buffer == nullptr) {
            return Err(ErrorCode::FailedToAcquireResource,
                "Prepared draw index buffer is no longer active");
        }

        ResolvedDraw native{
            .packet = draw.packet,
            .transform = draw.transform,
            .skin_matrices = draw.skin_matrices,
            .pipeline = pipeline,
            .index_buffer = index_buffer,
            .index_format = draw.geometry.index_format,
            .material = draw.material,
        };
        native.vertex_buffers.reserve(draw.geometry.vertex_buffers.size());
        for (const BufferHandle buffer : draw.geometry.vertex_buffers) {
            const rhi::Buffer* vertex_buffer = resources.Resolve(buffer);
            if (vertex_buffer == nullptr) {
                return Err(ErrorCode::FailedToAcquireResource,
                    "Prepared draw vertex buffer is no longer active");
            }
            native.vertex_buffers.push_back(vertex_buffer);
        }
        resolved.draws.push_back(std::move(native));
    }

    u32 expected_first_draw = 0;
    for (const auto& batch : prepared.batches) {
        const u64 batch_end = static_cast<u64>(batch.first_draw) + batch.draw_count;
        if (batch.draw_count == 0 || batch.first_draw != expected_first_draw ||
            batch_end > prepared.draws.size()) {
            return Err(ErrorCode::ValidationInvalidState,
                "Prepared draw batches do not form contiguous valid ranges");
        }
        for (u32 index = batch.first_draw; index < batch_end; ++index) {
            if (prepared.draws[index].pipeline != batch.pipeline) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Prepared draw batch contains a mismatched pipeline");
            }
        }
        const rhi::RenderPipeline* pipeline = pipelines.Resolve(batch.pipeline);
        if (pipeline == nullptr) {
            return Err(ErrorCode::FailedToAcquireResource,
                "Prepared draw batch pipeline is no longer active");
        }
        resolved.batches.push_back({
            .pipeline = pipeline,
            .first_draw = batch.first_draw,
            .draw_count = batch.draw_count,
        });
        expected_first_draw = static_cast<u32>(batch_end);
    }
    if (expected_first_draw != prepared.draws.size()) {
        return Err(
            ErrorCode::ValidationInvalidState, "Prepared draw batches do not cover every draw");
    }
    return Ok(std::move(resolved));
}

} // namespace woki::gfx
