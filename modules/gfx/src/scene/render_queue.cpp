#include <woki/gfx/scene/render_queue.hpp>

#include <limits>
#include <unordered_map>

namespace woki::gfx {
namespace {

[[nodiscard]] bool MatchesFilter(
    const DrawPacket& draw, const ExtractedObject& object, const RenderQueueFilter& filter) {
    return (!filter.phase || draw.phase == *filter.phase) &&
           (object.layer_mask & filter.layer_mask) != 0 &&
           (!filter.shadow_casters_only || object.casts_shadows);
}

[[nodiscard]] bool CanAppend(const DrawBatch& batch, const DrawPacket& draw) {
    return batch.mesh == draw.mesh && batch.material == draw.material && batch.phase == draw.phase;
}

} // namespace

Result<RenderQueue> BuildRenderQueue(
    const RenderSnapshot& snapshot, const RenderQueueFilter& filter) {
    if (snapshot.draws.size() > std::numeric_limits<u32>::max()) {
        return Err(ErrorCode::ValidationOutOfRange, "Render queue exceeds the draw index range");
    }

    std::unordered_map<u64, const ExtractedObject*> objects{};
    objects.reserve(snapshot.objects.size());
    for (const auto& object : snapshot.objects) {
        if (!object.object || !objects.emplace(object.object.Value(), &object).second) {
            return Err(ErrorCode::ValidationInvalidState,
                "Render snapshot contains an invalid or duplicate object");
        }
    }

    RenderQueue queue{.snapshot_sequence = snapshot.sequence};
    queue.draws.reserve(snapshot.draws.size());
    queue.batches.reserve(snapshot.draws.size());

    for (const auto& draw : snapshot.draws) {
        const auto object = objects.find(draw.object.Value());
        if (!draw.object || object == objects.end()) {
            return Err(ErrorCode::ValidationInvalidState,
                "Draw packet does not reference an extracted object");
        }
        if (!draw.mesh || !draw.material || draw.index_count == 0) {
            return Err(ErrorCode::ValidationInvalidState,
                "Draw packet contains an invalid resource or empty index range");
        }
        if (draw.mesh != object->second->mesh) {
            return Err(ErrorCode::ValidationInvalidState,
                "Draw packet mesh does not match its extracted object");
        }
        if (!MatchesFilter(draw, *object->second, filter)) {
            continue;
        }

        const auto draw_index = static_cast<u32>(queue.draws.size());
        queue.draws.push_back({.packet = draw, .transform = object->second->transform});
        if (queue.batches.empty() || !CanAppend(queue.batches.back(), draw)) {
            queue.batches.push_back({
                .mesh = draw.mesh,
                .material = draw.material,
                .phase = draw.phase,
                .first_draw = draw_index,
                .draw_count = 1,
            });
        } else {
            ++queue.batches.back().draw_count;
        }
    }

    return Ok(std::move(queue));
}

} // namespace woki::gfx
