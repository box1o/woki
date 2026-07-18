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

TEST_CASE("Render graph tracks transient buffer dependencies and lifetimes") {
    woki::gfx::RenderGraph graph{};
    const auto buffer = graph.AddTransientBuffer({
        .label = "Visible draws",
        .size = 4096,
        .usage = woki::rhi::BufferUsage::Storage | woki::rhi::BufferUsage::CopyDst,
    });
    REQUIRE(buffer);
    const auto producer = graph.AddPass({
        .label = "Build visible draws",
        .resources = {{.resource = *buffer, .access = woki::gfx::GraphAccess::Write}},
        .buffers = {{.resource = *buffer}},
    });
    const auto consumer = graph.AddPass({
        .label = "Consume visible draws",
        .resources = {{.resource = *buffer, .access = woki::gfx::GraphAccess::Read}},
        .buffers = {{.resource = *buffer}},
    });
    REQUIRE(producer);
    REQUIRE(consumer);

    const auto compiled = graph.Compile();
    REQUIRE(compiled);
    REQUIRE(compiled->passes.back().dependencies == std::vector{*producer});
    REQUIRE(compiled->lifetimes.size() == 1);
    REQUIRE(compiled->lifetimes.front().first_pass == 0);
    REQUIRE(compiled->lifetimes.front().last_pass == 1);
}

TEST_CASE("Render graph validates declared buffer inputs") {
    woki::gfx::RenderGraph graph{};
    const auto buffer = graph.AddPerFrameBuffer("Frame instances");
    const auto texture = graph.AddPerFrameTexture("Frame color");
    REQUIRE(buffer);
    REQUIRE(texture);

    REQUIRE(graph.AddPass({
        .label = "Missing access",
        .buffers = {{.resource = *buffer}},
    }));
    REQUIRE_FALSE(graph.Compile());

    graph.Clear();
    const auto color = graph.AddPerFrameTexture("Wrong kind");
    REQUIRE(color);
    REQUIRE(graph.AddPass({
        .label = "Wrong kind",
        .resources = {{.resource = *color, .access = woki::gfx::GraphAccess::Read}},
        .buffers = {{.resource = *color}},
    }));
    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph permits per-frame and imported buffers to be read first") {
    woki::gfx::RenderGraph graph{};
    const auto per_frame = graph.AddPerFrameBuffer("Frame data");
    const auto imported = graph.Import(woki::gfx::BufferHandle::FromParts(1, 1), "Scene data");
    REQUIRE(per_frame);
    REQUIRE(imported);
    REQUIRE(graph.AddPass({
        .label = "Read external buffers",
        .resources =
            {
                {.resource = *per_frame, .access = woki::gfx::GraphAccess::Read},
                {.resource = *imported, .access = woki::gfx::GraphAccess::Read},
            },
        .buffers = {{.resource = *per_frame}, {.resource = *imported}},
    }));

    REQUIRE(graph.Compile());
}

TEST_CASE("Render graph schedules compute-produced buffers before render consumers") {
    woki::gfx::RenderGraph graph{};
    const auto buffer = graph.AddTransientBuffer({
        .label = "Indirect commands",
        .size = 1024,
        .usage = woki::rhi::BufferUsage::Storage | woki::rhi::BufferUsage::Indirect,
    });
    REQUIRE(buffer);
    const auto compute = graph.AddPass({
        .label = "Build commands",
        .kind = woki::gfx::GraphPassKind::Compute,
        .resources = {{.resource = *buffer, .access = woki::gfx::GraphAccess::Write}},
        .buffers = {{.resource = *buffer}},
        .compute_execute = [](woki::rhi::ComputePassContext&) { return woki::Ok(); },
    });
    const auto render = graph.AddPass({
        .label = "Draw commands",
        .resources = {{.resource = *buffer, .access = woki::gfx::GraphAccess::Read}},
        .buffers = {{.resource = *buffer}},
    });
    REQUIRE(compute);
    REQUIRE(render);

    const auto compiled = graph.Compile();
    REQUIRE(compiled);
    REQUIRE(compiled->passes.back().dependencies == std::vector{*compute});
}

TEST_CASE("Render graph rejects render attachments on compute passes") {
    woki::gfx::RenderGraph graph{};
    const auto output = graph.AddPerFrameTexture("Output");
    REQUIRE(output);
    REQUIRE(graph.AddPass({
        .label = "Invalid compute",
        .kind = woki::gfx::GraphPassKind::Compute,
        .resources = {{.resource = *output, .access = woki::gfx::GraphAccess::Write}},
        .colors = {{.resource = *output}},
    }));
    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph schedules compute storage textures before sampled consumers") {
    woki::gfx::RenderGraph graph{};
    const auto texture = graph.AddTransientTexture({
        .label = "Bloom image",
        .format = woki::rhi::TextureFormat::RGBA16Float,
        .usage = woki::rhi::TextureUsage::StorageBinding |
                 woki::rhi::TextureUsage::TextureBinding,
    });
    REQUIRE(texture);
    const auto compute = graph.AddPass({
        .label = "Compute bloom",
        .kind = woki::gfx::GraphPassKind::Compute,
        .resources = {{.resource = *texture, .access = woki::gfx::GraphAccess::Write}},
        .storage_textures = {{.resource = *texture}},
        .compute_execute = [](woki::rhi::ComputePassContext&) { return woki::Ok(); },
    });
    const auto composite = graph.AddPass({
        .label = "Composite bloom",
        .resources = {{.resource = *texture, .access = woki::gfx::GraphAccess::Read}},
        .samples = {{.resource = *texture}},
    });
    REQUIRE(compute);
    REQUIRE(composite);

    const auto compiled = graph.Compile();
    REQUIRE(compiled);
    REQUIRE(compiled->passes.back().dependencies == std::vector{*compute});
    REQUIRE(compiled->lifetimes.front().first_pass == 0);
    REQUIRE(compiled->lifetimes.front().last_pass == 1);
}

TEST_CASE("Render graph validates storage texture declarations") {
    woki::gfx::RenderGraph graph{};
    const auto texture = graph.AddTransientTexture({
        .label = "Storage output",
        .format = woki::rhi::TextureFormat::RGBA8Unorm,
        .usage = woki::rhi::TextureUsage::TextureBinding,
    });
    REQUIRE(texture);
    REQUIRE(graph.AddPass({
        .label = "Missing storage usage",
        .kind = woki::gfx::GraphPassKind::Compute,
        .resources = {{.resource = *texture, .access = woki::gfx::GraphAccess::Write}},
        .storage_textures = {{.resource = *texture}},
        .compute_execute = [](woki::rhi::ComputePassContext&) { return woki::Ok(); },
    }));
    REQUIRE_FALSE(graph.Compile());

    graph.Clear();
    const auto buffer = graph.AddTransientBuffer({
        .label = "Wrong storage kind",
        .size = 256,
        .usage = woki::rhi::BufferUsage::Storage,
    });
    REQUIRE(buffer);
    REQUIRE(graph.AddPass({
        .label = "Wrong storage kind",
        .kind = woki::gfx::GraphPassKind::Compute,
        .resources = {{.resource = *buffer, .access = woki::gfx::GraphAccess::Write}},
        .storage_textures = {{.resource = *buffer}},
        .compute_execute = [](woki::rhi::ComputePassContext&) { return woki::Ok(); },
    }));
    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph tracks multisampled color resolves") {
    woki::gfx::RenderGraph graph{};
    const auto multisampled = graph.AddTransientTexture({
        .label = "MSAA color",
        .format = woki::rhi::TextureFormat::RGBA16Float,
        .usage = woki::rhi::TextureUsage::RenderAttachment,
        .sample_count = 4,
    });
    const auto resolved = graph.AddTransientTexture({
        .label = "Resolved color",
        .format = woki::rhi::TextureFormat::RGBA16Float,
        .usage = woki::rhi::TextureUsage::RenderAttachment |
                 woki::rhi::TextureUsage::TextureBinding,
    });
    REQUIRE(multisampled);
    REQUIRE(resolved);
    REQUIRE(graph.AddPass({
        .label = "Resolve scene color",
        .resources =
            {
                {.resource = *multisampled, .access = woki::gfx::GraphAccess::Write},
                {.resource = *resolved, .access = woki::gfx::GraphAccess::Write},
            },
        .colors = {{.resource = *multisampled, .resolve = *resolved}},
    }));

    const auto compiled = graph.Compile();
    REQUIRE(compiled);
    REQUIRE(compiled->lifetimes.size() == 2);
}

TEST_CASE("Render graph rejects invalid multisample resolves") {
    woki::gfx::RenderGraph graph{};
    const auto source = graph.AddTransientTexture({
        .label = "Single-sampled source",
        .format = woki::rhi::TextureFormat::RGBA8Unorm,
        .usage = woki::rhi::TextureUsage::RenderAttachment,
    });
    const auto target = graph.AddTransientTexture({
        .label = "Resolve target",
        .format = woki::rhi::TextureFormat::RGBA8Unorm,
        .usage = woki::rhi::TextureUsage::RenderAttachment,
    });
    REQUIRE(source);
    REQUIRE(target);
    REQUIRE(graph.AddPass({
        .label = "Invalid resolve",
        .resources =
            {
                {.resource = *source, .access = woki::gfx::GraphAccess::Write},
                {.resource = *target, .access = woki::gfx::GraphAccess::Write},
            },
        .colors = {{.resource = *source, .resolve = *target}},
    }));
    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph schedules texture copy passes between producers and consumers") {
    woki::gfx::RenderGraph graph{};
    const auto source = graph.AddTransientTexture({
        .label = "Rendered image",
        .format = woki::rhi::TextureFormat::RGBA16Float,
        .usage = woki::rhi::TextureUsage::RenderAttachment |
                 woki::rhi::TextureUsage::CopySrc,
    });
    const auto destination = graph.AddTransientTexture({
        .label = "History image",
        .format = woki::rhi::TextureFormat::RGBA16Float,
        .usage = woki::rhi::TextureUsage::CopyDst |
                 woki::rhi::TextureUsage::TextureBinding,
    });
    REQUIRE(source);
    REQUIRE(destination);
    const auto producer = graph.AddPass({
        .label = "Render image",
        .resources = {{.resource = *source, .access = woki::gfx::GraphAccess::Write}},
        .colors = {{.resource = *source}},
    });
    const auto copy = graph.AddPass({
        .label = "Store history",
        .kind = woki::gfx::GraphPassKind::Copy,
        .resources =
            {
                {.resource = *source, .access = woki::gfx::GraphAccess::Read},
                {.resource = *destination, .access = woki::gfx::GraphAccess::Write},
            },
        .copies = {{.source = *source, .destination = *destination}},
    });
    const auto consumer = graph.AddPass({
        .label = "Read history",
        .resources = {{.resource = *destination, .access = woki::gfx::GraphAccess::Read}},
        .samples = {{.resource = *destination}},
    });
    REQUIRE(producer);
    REQUIRE(copy);
    REQUIRE(consumer);

    const auto compiled = graph.Compile();
    REQUIRE(compiled);
    REQUIRE(compiled->passes[1].dependencies == std::vector{*producer});
    REQUIRE(compiled->passes[2].dependencies == std::vector{*copy});
    REQUIRE(compiled->lifetimes.size() == 2);
    REQUIRE(compiled->lifetimes[0].last_pass == 1);
    REQUIRE(compiled->lifetimes[1].first_pass == 1);
}

TEST_CASE("Render graph validates texture copy usage and access") {
    woki::gfx::RenderGraph graph{};
    const auto source = graph.AddTransientTexture({
        .label = "Source without copy usage",
        .format = woki::rhi::TextureFormat::RGBA8Unorm,
        .usage = woki::rhi::TextureUsage::RenderAttachment,
    });
    const auto destination = graph.AddTransientTexture({
        .label = "Destination without copy usage",
        .format = woki::rhi::TextureFormat::RGBA8Unorm,
        .usage = woki::rhi::TextureUsage::TextureBinding,
    });
    REQUIRE(source);
    REQUIRE(destination);
    REQUIRE(graph.AddPass({
        .label = "Invalid copy usages",
        .kind = woki::gfx::GraphPassKind::Copy,
        .resources =
            {
                {.resource = *source, .access = woki::gfx::GraphAccess::Read},
                {.resource = *destination, .access = woki::gfx::GraphAccess::Write},
            },
        .copies = {{.source = *source, .destination = *destination}},
    }));
    REQUIRE_FALSE(graph.Compile());

    graph.Clear();
    const auto per_frame = graph.AddPerFrameTexture("Swapchain output");
    const auto imported = graph.Import(woki::gfx::TextureHandle::FromParts(1, 1));
    REQUIRE(per_frame);
    REQUIRE(imported);
    REQUIRE(graph.AddPass({
        .label = "Invalid per-frame copy",
        .kind = woki::gfx::GraphPassKind::Copy,
        .resources =
            {
                {.resource = *imported, .access = woki::gfx::GraphAccess::Read},
                {.resource = *per_frame, .access = woki::gfx::GraphAccess::Write},
            },
        .copies = {{.source = *imported, .destination = *per_frame}},
    }));
    REQUIRE_FALSE(graph.Compile());
}

TEST_CASE("Render graph rejects unused transient resources") {
    woki::gfx::RenderGraph graph{};
    REQUIRE(graph.AddTransientBuffer({
        .label = "Unused",
        .size = 256,
        .usage = woki::rhi::BufferUsage::Storage,
    }));

    REQUIRE_FALSE(graph.Compile());
}
