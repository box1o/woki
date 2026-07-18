#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/graphs.hpp>

TEST_CASE("Render graph derives resource dependencies and lifetimes") {
    woki::gfx::RenderGraph graph{};
    auto color = graph.AddTransientTexture({.label = "HDR color",
        .format = woki::rhi::TextureFormat::RGBA16Float,
        .usage =
            woki::rhi::TextureUsage::RenderAttachment | woki::rhi::TextureUsage::TextureBinding});
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
    auto transient = graph.AddTransientTexture({.label = "Transient",
        .format = woki::rhi::TextureFormat::RGBA8Unorm,
        .usage = woki::rhi::TextureUsage::TextureBinding});
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
    auto target = graph.AddTransientTexture({.label = "Target",
        .format = woki::rhi::TextureFormat::RGBA8Unorm,
        .usage =
            woki::rhi::TextureUsage::RenderAttachment | woki::rhi::TextureUsage::TextureBinding});
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

TEST_CASE("Render graph validates attachment access declarations") {
    woki::gfx::RenderGraph graph{};
    auto color = graph.AddPerFrameTexture("Output");
    REQUIRE(color);
    REQUIRE(graph.AddPass({
        .label = "Invalid output",
        .resources = {{.resource = *color, .access = woki::gfx::GraphAccess::Read}},
        .colors = {{.resource = *color}},
    }));

    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph accepts executable per-frame color outputs") {
    woki::gfx::RenderGraph graph{};
    auto color = graph.AddPerFrameTexture("Output");
    REQUIRE(color);
    REQUIRE(graph.AddPass({
        .label = "Output pass",
        .resources = {{.resource = *color, .access = woki::gfx::GraphAccess::Write}},
        .colors = {{.resource = *color}},
        .execute = [](woki::rhi::RenderPassContext&) { return woki::Ok(); },
    }));

    auto compiled = graph.Compile();
    REQUIRE(compiled);
    REQUIRE(compiled->passes.size() == 1);
}
