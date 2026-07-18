#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/graphs.hpp>

TEST_CASE("Render graph derives resource dependencies and lifetimes") {
    woki::gfx::RenderGraph graph{};
    auto color = graph.AddResource({.label = "HDR color"});
    REQUIRE(color);

    auto geometry = graph.AddPass({
        .label = "Geometry",
        .resources = {{.resource = *color, .access = woki::gfx::GraphAccess::Write}},
    });
    auto tonemap = graph.AddPass({
        .label = "Tonemap",
        .resources = {{.resource = *color, .access = woki::gfx::GraphAccess::Read}},
    });
    REQUIRE(geometry);
    REQUIRE(tonemap);

    auto compiled = graph.Compile();
    REQUIRE(compiled);
    REQUIRE(compiled->passes.size() == 2);
    REQUIRE(compiled->passes[0].pass == *geometry);
    REQUIRE(compiled->passes[1].pass == *tonemap);
    REQUIRE(compiled->passes[1].dependencies == std::vector{*geometry});
    REQUIRE(compiled->lifetimes.size() == 1);
    REQUIRE(compiled->lifetimes[0].first_pass == 0);
    REQUIRE(compiled->lifetimes[0].last_pass == 1);
}

TEST_CASE("Render graph permits imported resources to be read first") {
    woki::gfx::RenderGraph graph{};
    auto source = graph.Import(woki::gfx::TextureHandle::FromParts(1, 1));
    REQUIRE(source);
    REQUIRE(graph.AddPass({
        .label = "Copy",
        .resources = {{.resource = *source, .access = woki::gfx::GraphAccess::Read}},
    }));

    REQUIRE(graph.Compile());
}

TEST_CASE("Render graph rejects transient resources read before a write") {
    woki::gfx::RenderGraph graph{};
    auto transient = graph.AddResource({.label = "Transient"});
    REQUIRE(transient);
    REQUIRE(graph.AddPass({
        .label = "Invalid reader",
        .resources = {{.resource = *transient, .access = woki::gfx::GraphAccess::Read}},
    }));

    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph detects explicit dependency cycles") {
    woki::gfx::RenderGraph graph{};
    auto first =
        graph.AddPass({.label = "First", .depends_on = {woki::gfx::GraphPass::FromIndex(1)}});
    auto second = graph.AddPass({.label = "Second", .depends_on = {*first}});
    REQUIRE(first);
    REQUIRE(second);

    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph rejects duplicate resource declarations in a pass") {
    woki::gfx::RenderGraph graph{};
    auto target = graph.AddResource({.label = "Target"});
    REQUIRE(target);
    REQUIRE(graph.AddPass({
        .label = "Duplicate",
        .resources =
            {
                {.resource = *target, .access = woki::gfx::GraphAccess::Write},
                {.resource = *target, .access = woki::gfx::GraphAccess::Read},
            },
    }));

    REQUIRE_FALSE(graph.Compile());
}
