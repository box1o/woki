#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/graphs.hpp>

namespace {

const woki::StringId kColor{"frame.color"};

class ProducerFeature final : public woki::gfx::RenderFeature {
public:
    [[nodiscard]] std::string_view Name() const noexcept override { return "Producer"; }

    [[nodiscard]] woki::Result<void> AddPasses(woki::gfx::RenderGraph& graph,
        woki::gfx::RenderGraphBlackboard& blackboard,
        const woki::gfx::RenderFeatureContext&) override {
        auto color = graph.AddResource({.label = "Frame color"});
        if (!color) {
            return woki::Err(color.error());
        }
        if (auto published = blackboard.Publish(kColor, *color); !published) {
            return published;
        }
        auto pass = graph.AddPass({
            .label = "Produce color",
            .resources = {{.resource = *color, .access = woki::gfx::GraphAccess::Write}},
        });
        return pass ? woki::Ok() : woki::Err(pass.error());
    }
};

class ConsumerFeature final : public woki::gfx::RenderFeature {
public:
    [[nodiscard]] std::string_view Name() const noexcept override { return "Consumer"; }

    [[nodiscard]] woki::Result<void> AddPasses(woki::gfx::RenderGraph& graph,
        woki::gfx::RenderGraphBlackboard& blackboard,
        const woki::gfx::RenderFeatureContext&) override {
        const auto color = blackboard.Find(kColor);
        if (!color) {
            return woki::Err(
                woki::ErrorCode::FailedToAcquireResource, "Frame color was not published");
        }
        auto pass = graph.AddPass({
            .label = "Consume color",
            .resources = {{.resource = color, .access = woki::gfx::GraphAccess::Read}},
        });
        return pass ? woki::Ok() : woki::Err(pass.error());
    }
};

[[nodiscard]] woki::gfx::RenderFeatureContext EmptyContext() {
    static const woki::gfx::RenderSnapshot snapshot{};
    static const woki::gfx::RenderQueue queue{};
    return {.snapshot = snapshot, .opaque_queue = queue, .transparent_queue = queue};
}

} // namespace

TEST_CASE("Render features build a shared graph in registration order") {
    woki::gfx::RenderFeatureRegistry features{};
    REQUIRE(features.Add(std::make_unique<ProducerFeature>()));
    REQUIRE(features.Add(std::make_unique<ConsumerFeature>()));

    auto result = features.Build(EmptyContext());

    REQUIRE(result);
    REQUIRE(result->graph.PassCount() == 2);
    REQUIRE(result->blackboard.Contains(kColor));
    REQUIRE(result->compiled.passes.size() == 2);
    REQUIRE(result->compiled.passes.back().dependencies.size() == 1);
}

TEST_CASE("Disabled render features do not contribute passes") {
    woki::gfx::RenderFeatureRegistry features{};
    REQUIRE(features.Add(std::make_unique<ProducerFeature>()));
    REQUIRE(features.SetEnabled("Producer", false));

    auto result = features.Build(EmptyContext());

    REQUIRE(result);
    REQUIRE(result->graph.PassCount() == 0);
    REQUIRE(result->blackboard.Size() == 0);
}

TEST_CASE("Render feature names must be unique") {
    woki::gfx::RenderFeatureRegistry features{};
    REQUIRE(features.Add(std::make_unique<ProducerFeature>()));
    REQUIRE_FALSE(features.Add(std::make_unique<ProducerFeature>()));
    REQUIRE(features.Size() == 1);
}

TEST_CASE("Removing a render feature transfers its ownership") {
    woki::gfx::RenderFeatureRegistry features{};
    REQUIRE(features.Add(std::make_unique<ProducerFeature>()));

    auto removed = features.Remove("Producer");

    REQUIRE(removed);
    REQUIRE(removed->Name() == "Producer");
    REQUIRE(features.Size() == 0);
}
