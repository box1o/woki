#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/frames.hpp>

namespace {

constexpr auto kObject = woki::gfx::RenderObjectHandle::FromParts(1, 1);
constexpr auto kMesh = woki::gfx::MeshHandle::FromParts(1, 1);
constexpr auto kOpaqueMaterial = woki::gfx::MaterialHandle::FromParts(1, 1);
constexpr auto kTransparentMaterial = woki::gfx::MaterialHandle::FromParts(2, 1);

class QueueFeature final : public woki::gfx::RenderFeature {
public:
    [[nodiscard]] std::string_view Name() const noexcept override { return "Queue feature"; }

    [[nodiscard]] woki::Result<void> AddPasses(woki::gfx::RenderGraph& graph,
        woki::gfx::RenderGraphBlackboard&,
        const woki::gfx::RenderFeatureContext& context) override {
        observed_sequence = context.snapshot.sequence;
        observed_view_position = context.view.world_position;
        opaque_draws = context.opaque_queue.draws.size();
        transparent_draws = context.transparent_queue.draws.size();
        auto pass = graph.AddPass({.label = "Queue pass"});
        return pass ? woki::Ok() : woki::Err(pass.error());
    }

    woki::u64 observed_sequence{0};
    std::size_t opaque_draws{0};
    std::size_t transparent_draws{0};
    woki::math::vec3f observed_view_position{};
};

[[nodiscard]] woki::gfx::RenderSnapshot MakeSnapshot() {
    return {
        .sequence = 12,
        .objects = {{.object = kObject, .mesh = kMesh}},
        .draws =
            {
                {.object = kObject, .mesh = kMesh, .material = kOpaqueMaterial, .index_count = 3},
                {.object = kObject,
                    .mesh = kMesh,
                    .material = kTransparentMaterial,
                    .index_count = 3,
                    .phase = woki::gfx::DrawPhase::Transparent},
            },
    };
}

} // namespace

TEST_CASE("Frame planner builds phase queues and feature graph from one snapshot") {
    woki::gfx::RenderFeatureRegistry features{};
    auto feature = std::make_unique<QueueFeature>();
    auto* observer = feature.get();
    REQUIRE(features.Add(std::move(feature)));

    const woki::gfx::RenderView view{.world_position = {1.0F, 2.0F, 3.0F}};
    auto plan = woki::gfx::BuildRenderFramePlan(MakeSnapshot(), features, view);

    REQUIRE(plan);
    REQUIRE(plan->snapshot.sequence == 12);
    REQUIRE(plan->opaque_queue.snapshot_sequence == 12);
    REQUIRE(plan->transparent_queue.snapshot_sequence == 12);
    REQUIRE(plan->opaque_queue.draws.size() == 1);
    REQUIRE(plan->transparent_queue.draws.size() == 1);
    REQUIRE(plan->feature_graph.compiled.passes.size() == 1);
    REQUIRE(observer->observed_sequence == 12);
    REQUIRE(observer->opaque_draws == 1);
    REQUIRE(observer->transparent_draws == 1);
    REQUIRE(observer->observed_view_position == view.world_position);
}

TEST_CASE("Frame planner rejects an inconsistent snapshot before invoking features") {
    woki::gfx::RenderFeatureRegistry features{};
    auto feature = std::make_unique<QueueFeature>();
    auto* observer = feature.get();
    REQUIRE(features.Add(std::move(feature)));
    auto snapshot = MakeSnapshot();
    snapshot.draws.front().mesh = woki::gfx::MeshHandle::FromParts(9, 1);

    auto plan = woki::gfx::BuildRenderFramePlan(std::move(snapshot), features);

    REQUIRE_FALSE(plan);
    REQUIRE(observer->observed_sequence == 0);
}

TEST_CASE("Frame planner culls bounded objects outside the supplied frustum") {
    woki::gfx::RenderFeatureRegistry features{};
    auto feature = std::make_unique<QueueFeature>();
    auto* observer = feature.get();
    REQUIRE(features.Add(std::move(feature)));
    auto snapshot = MakeSnapshot();
    snapshot.objects.front().bounds =
        woki::gfx::BoundingSphere{.center = {4.0F, 0.0F, 0.0F}, .radius = 0.25F};
    const auto frustum = woki::gfx::ExtractFrustum(woki::math::mat4f::identity());
    REQUIRE(frustum);

    auto plan =
        woki::gfx::BuildRenderFramePlan(std::move(snapshot), features, {.frustum = *frustum});

    REQUIRE(plan);
    REQUIRE(plan->snapshot.objects.empty());
    REQUIRE(observer->opaque_draws == 0);
    REQUIRE(observer->transparent_draws == 0);
}
