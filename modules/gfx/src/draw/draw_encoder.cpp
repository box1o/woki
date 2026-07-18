#include <woki/gfx/draw/draw_encoder.hpp>

#include <limits>

namespace woki::gfx {

Result<void> Validate(const ResolvedDrawList& draws) {
    for (const auto& draw : draws.draws) {
        if (draw.pipeline == nullptr || draw.index_buffer == nullptr ||
            draw.vertex_buffers.empty()) {
            return Err(ErrorCode::ValidationNullValue,
                "Resolved draw requires pipeline and geometry resources");
        }
        if (draw.index_format == rhi::IndexFormat::Undefined || draw.packet.index_count == 0) {
            return Err(ErrorCode::ValidationInvalidState,
                "Resolved draw requires a valid nonempty index range");
        }
        for (const rhi::Buffer* buffer : draw.vertex_buffers) {
            if (buffer == nullptr) {
                return Err(ErrorCode::ValidationNullValue,
                    "Resolved draw contains an invalid vertex buffer");
            }
        }
        if (draw.vertex_buffers.size() > std::numeric_limits<u32>::max()) {
            return Err(ErrorCode::ValidationOutOfRange,
                "Resolved draw exceeds the vertex buffer slot range");
        }
    }

    u32 expected_first_draw = 0;
    for (const auto& batch : draws.batches) {
        const u64 end = static_cast<u64>(batch.first_draw) + batch.draw_count;
        if (batch.pipeline == nullptr || batch.draw_count == 0 ||
            batch.first_draw != expected_first_draw || end > draws.draws.size()) {
            return Err(ErrorCode::ValidationInvalidState,
                "Resolved draw batches do not form contiguous valid ranges");
        }
        const auto& first = draws.draws[batch.first_draw];
        for (u32 index = batch.first_draw; index < end; ++index) {
            if (draws.draws[index].pipeline != batch.pipeline) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Resolved draw batch contains a mismatched pipeline");
            }
            if (draws.draws[index].index_buffer != first.index_buffer ||
                draws.draws[index].index_format != first.index_format ||
                draws.draws[index].vertex_buffers != first.vertex_buffers) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Resolved draw batch contains mismatched geometry");
            }
        }
        expected_first_draw = static_cast<u32>(end);
    }
    if (expected_first_draw != draws.draws.size()) {
        return Err(
            ErrorCode::ValidationInvalidState, "Resolved draw batches do not cover every draw");
    }
    return Ok();
}

Result<DrawEncodingStats> EncodeDraws(rhi::RenderPassEncoder& pass, const ResolvedDrawList& draws,
    DrawBindingEncoder* const bindings) {
    if (auto validation = Validate(draws); !validation) {
        return Err(validation.error());
    }
    if (bindings != nullptr) {
        if (auto prepared = bindings->Prepare(draws); !prepared) {
            return Err(prepared.error());
        }
    }

    return EncodePreparedDraws(pass, draws, bindings);
}

Result<DrawEncodingStats> EncodePreparedDraws(rhi::RenderPassEncoder& pass,
    const ResolvedDrawList& draws, DrawBindingEncoder* const bindings) {
    if (auto validation = Validate(draws); !validation) {
        return Err(validation.error());
    }

    DrawEncodingStats stats{};
    const rhi::RenderPipeline* active_pipeline = nullptr;
    const rhi::Buffer* active_index = nullptr;
    rhi::IndexFormat active_index_format{rhi::IndexFormat::Undefined};
    std::vector<const rhi::Buffer*> active_vertices{};

    for (const auto& batch : draws.batches) {
        if (active_pipeline != batch.pipeline) {
            pass.SetPipeline(*batch.pipeline);
            active_pipeline = batch.pipeline;
            ++stats.pipeline_bindings;
        }

        const auto& first = draws.draws[batch.first_draw];
        if (active_index != first.index_buffer || active_index_format != first.index_format ||
            active_vertices != first.vertex_buffers) {
            pass.SetIndexBuffer(*first.index_buffer, first.index_format);
            for (u32 slot = 0; slot < first.vertex_buffers.size(); ++slot) {
                pass.SetVertexBuffer(slot, first.vertex_buffers[slot]);
            }
            for (u32 slot = static_cast<u32>(first.vertex_buffers.size());
                slot < active_vertices.size(); ++slot) {
                pass.SetVertexBuffer(slot, nullptr);
            }
            active_index = first.index_buffer;
            active_index_format = first.index_format;
            active_vertices = first.vertex_buffers;
            ++stats.geometry_bindings;
        }

        const u32 end = batch.first_draw + batch.draw_count;
        for (u32 index = batch.first_draw; index < end; ++index) {
            const auto& draw = draws.draws[index];
            if (bindings != nullptr) {
                bindings->Encode(pass, draw, index);
                ++stats.binding_updates;
            }
            pass.DrawIndexed(
                draw.packet.index_count, 1, draw.packet.first_index, draw.packet.base_vertex, 0);
            ++stats.draw_calls;
        }
    }
    return Ok(stats);
}

} // namespace woki::gfx
