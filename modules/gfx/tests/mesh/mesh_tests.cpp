#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/meshes.hpp>

TEST_CASE("Mesh descriptions validate vertex and index storage") {
    woki::gfx::MeshDesc desc{};
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());

    desc.vertex_layout = woki::StringId{"position"};
    desc.vertex_streams.push_back({
        .stride = 12,
        .vertex_count = 3,
        .data = std::vector<std::byte>(36),
    });
    desc.index_format = woki::rhi::IndexFormat::Uint16;
    desc.index_count = 3;
    desc.index_data.resize(6);
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.vertex_streams.front().data.resize(35);
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Mesh vertex streams require matching vertex counts") {
    woki::gfx::MeshDesc desc{};
    desc.vertex_layout = woki::StringId{"position_uv"};
    desc.vertex_streams.push_back({
        .stride = 12,
        .vertex_count = 3,
        .data = std::vector<std::byte>(36),
    });
    desc.vertex_streams.push_back({
        .stride = 8,
        .vertex_count = 2,
        .data = std::vector<std::byte>(16),
    });
    desc.index_format = woki::rhi::IndexFormat::Uint32;
    desc.index_count = 3;
    desc.index_data.resize(12);

    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Mesh submeshes remain inside the index buffer") {
    woki::gfx::MeshDesc desc{};
    desc.vertex_layout = woki::StringId{"position"};
    desc.vertex_streams.push_back({
        .stride = 12,
        .vertex_count = 4,
        .data = std::vector<std::byte>(48),
    });
    desc.index_format = woki::rhi::IndexFormat::Uint32;
    desc.index_count = 6;
    desc.index_data.resize(24);
    desc.submeshes.push_back({.first_index = 0, .index_count = 6});
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.submeshes.front().first_index = 1;
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}
