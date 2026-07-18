#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/scenes.hpp>

TEST_CASE("Draw packets sort opaque before transparent") {
    std::vector<woki::gfx::DrawPacket> draws{
        {.material = woki::gfx::MaterialHandle::FromParts(1, 1),
            .phase = woki::gfx::DrawPhase::Transparent,
            .sort_depth = 10.0F},
        {.material = woki::gfx::MaterialHandle::FromParts(2, 1),
            .phase = woki::gfx::DrawPhase::Opaque},
    };

    woki::gfx::SortDrawPackets(draws);

    REQUIRE(draws.front().phase == woki::gfx::DrawPhase::Opaque);
    REQUIRE(draws.back().phase == woki::gfx::DrawPhase::Transparent);
}

TEST_CASE("Transparent draw packets sort back to front") {
    std::vector<woki::gfx::DrawPacket> draws{
        {.object = woki::gfx::RenderObjectHandle::FromParts(1, 1),
            .phase = woki::gfx::DrawPhase::Transparent,
            .sort_depth = 2.0F},
        {.object = woki::gfx::RenderObjectHandle::FromParts(2, 1),
            .phase = woki::gfx::DrawPhase::Transparent,
            .sort_depth = 8.0F},
    };

    woki::gfx::SortDrawPackets(draws);

    REQUIRE(draws.front().sort_depth == 8.0F);
    REQUIRE(draws.back().sort_depth == 2.0F);
}

TEST_CASE("Opaque draw packet sorting is deterministic") {
    const auto first_material = woki::gfx::MaterialHandle::FromParts(1, 1);
    const auto second_material = woki::gfx::MaterialHandle::FromParts(2, 1);
    std::vector<woki::gfx::DrawPacket> draws{
        {.object = woki::gfx::RenderObjectHandle::FromParts(2, 1), .material = second_material},
        {.object = woki::gfx::RenderObjectHandle::FromParts(1, 1), .material = first_material},
    };

    woki::gfx::SortDrawPackets(draws);

    REQUIRE(draws.front().material == first_material);
    REQUIRE(draws.back().material == second_material);
}
