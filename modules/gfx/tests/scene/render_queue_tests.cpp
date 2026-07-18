#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/scenes.hpp>

namespace {

constexpr auto kObject = woki::gfx::RenderObjectHandle::FromParts(1, 1);
constexpr auto kMesh = woki::gfx::MeshHandle::FromParts(1, 1);
constexpr auto kMaterial = woki::gfx::MaterialHandle::FromParts(1, 1);

[[nodiscard]] woki::gfx::RenderSnapshot MakeSnapshot() {
    return {
        .sequence = 42,
        .objects = {{.object = kObject,
            .mesh = kMesh,
            .skin_matrices = {woki::math::mat4f::identity()},
            .layer_mask = 0b0010}},
        .draws =
            {
                {.object = kObject, .mesh = kMesh, .material = kMaterial, .index_count = 3},
                {.object = kObject,
                    .mesh = kMesh,
                    .material = kMaterial,
                    .submesh = 1,
                    .index_count = 6},
            },
    };
}

} // namespace

TEST_CASE("Render queue preserves snapshot identity and batches adjacent draws") {
    auto queue = woki::gfx::BuildRenderQueue(MakeSnapshot());

    REQUIRE(queue);
    REQUIRE(queue->snapshot_sequence == 42);
    REQUIRE(queue->draws.size() == 2);
    REQUIRE(queue->draws.front().skin_matrices.size() == 1);
    REQUIRE(queue->batches.size() == 1);
    REQUIRE(queue->batches.front().first_draw == 0);
    REQUIRE(queue->batches.front().draw_count == 2);
}

TEST_CASE("Render queue filters draws by phase and layer") {
    auto snapshot = MakeSnapshot();
    snapshot.draws.back().phase = woki::gfx::DrawPhase::Transparent;

    auto queue = woki::gfx::BuildRenderQueue(
        snapshot, {.phase = woki::gfx::DrawPhase::Transparent, .layer_mask = 0b0010});
    REQUIRE(queue);
    REQUIRE(queue->draws.size() == 1);
    REQUIRE(queue->draws.front().packet.phase == woki::gfx::DrawPhase::Transparent);

    auto excluded = woki::gfx::BuildRenderQueue(snapshot, {.layer_mask = 0b0100});
    REQUIRE(excluded);
    REQUIRE(excluded->draws.empty());
}

TEST_CASE("Render queue rejects draw packets with missing objects") {
    auto snapshot = MakeSnapshot();
    snapshot.draws.front().object = woki::gfx::RenderObjectHandle::FromParts(2, 1);

    auto queue = woki::gfx::BuildRenderQueue(snapshot);

    REQUIRE_FALSE(queue);
}
