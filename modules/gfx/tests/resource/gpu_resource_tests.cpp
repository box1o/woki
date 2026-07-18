#include <catch2/catch_test_macros.hpp>

#include <cstddef>

#include <woki/gfx/resources.hpp>

TEST_CASE("Buffer resource descriptions validate size usage and initial data") {
    woki::gfx::BufferResourceDesc desc{};
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());

    desc.gpu.size = 64;
    desc.gpu.usage = woki::rhi::BufferUsage::Vertex;
    desc.initial_data.resize(32);
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.initial_data.resize(65);
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Transient resources reject retained CPU copies") {
    woki::gfx::BufferResourceDesc desc{};
    desc.gpu.size = 16;
    desc.gpu.usage = woki::rhi::BufferUsage::Uniform;
    desc.lifetime = woki::gfx::ResourceLifetime::Transient;
    desc.retain_cpu_copy = true;

    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Texture resource descriptions validate GPU properties") {
    woki::gfx::TextureResourceDesc desc{};
    desc.gpu.size = {.width = 1024, .height = 1024, .depth_or_array_layers = 1};
    desc.gpu.format = woki::rhi::TextureFormat::RGBA8Unorm;
    desc.gpu.usage = woki::rhi::TextureUsage::TextureBinding | woki::rhi::TextureUsage::CopyDst;

    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.gpu.mip_level_count = 11;
    REQUIRE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Texture upload descriptions validate subresource ranges and layout") {
    woki::gfx::TextureResourceDesc desc{};
    desc.gpu.size = {.width = 4, .height = 4, .depth_or_array_layers = 1};
    desc.gpu.format = woki::rhi::TextureFormat::RGBA8Unorm;
    desc.gpu.usage = woki::rhi::TextureUsage::TextureBinding | woki::rhi::TextureUsage::CopyDst;
    desc.initial_data.push_back({
        .data = std::vector<std::byte>(64),
        .mip_level = 0,
        .array_layer = 0,
        .bytes_per_row = 16,
        .rows_per_image = 4,
    });

    REQUIRE(woki::gfx::Validate(desc).has_value());
    desc.initial_data.front().mip_level = 1;
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Sampler resource descriptions validate LOD and anisotropy") {
    woki::gfx::SamplerResourceDesc desc{};
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.gpu.lod_min_clamp = 4.0F;
    desc.gpu.lod_max_clamp = 2.0F;
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}
