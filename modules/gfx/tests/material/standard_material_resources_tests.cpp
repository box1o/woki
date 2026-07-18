#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/materials.hpp>

TEST_CASE("Standard material resources require every fallback") {
    woki::gfx::StandardMaterialResources resources{};
    REQUIRE_FALSE(resources.Valid());

    resources.white = woki::gfx::TextureHandle::FromParts(0, 1);
    resources.black = woki::gfx::TextureHandle::FromParts(1, 1);
    resources.normal = woki::gfx::TextureHandle::FromParts(2, 1);
    resources.metallic_roughness = woki::gfx::TextureHandle::FromParts(3, 1);
    resources.sampler = woki::gfx::SamplerHandle::FromParts(0, 1);
    resources.shadow_sampler = woki::gfx::SamplerHandle::FromParts(1, 1);
    REQUIRE(resources.Valid());
}
