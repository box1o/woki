#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/draws.hpp>

TEST_CASE("Prepared draw batches preserve contiguous state ranges") {
    const auto pipeline = woki::gfx::PipelineHandle::FromParts(1, 1);
    const auto mesh = woki::gfx::MeshHandle::FromParts(2, 1);
    const auto material = woki::gfx::MaterialHandle::FromParts(3, 1);
    woki::gfx::PreparedDrawList prepared{
        .snapshot_sequence = 8,
        .batches = {{
            .pipeline = pipeline,
            .mesh = mesh,
            .material = material,
            .first_draw = 0,
            .draw_count = 2,
        }},
    };

    REQUIRE(prepared.snapshot_sequence == 8);
    REQUIRE(prepared.batches.front().pipeline == pipeline);
    REQUIRE(prepared.batches.front().first_draw == 0);
    REQUIRE(prepared.batches.front().draw_count == 2);
}

TEST_CASE("Resolved draw state owns the material snapshot") {
    woki::gfx::ResolvedDraw draw{};
    draw.material.label = "Stable material";
    draw.material.parameters.Set(woki::gfx::material_parameters::kRoughness, 0.4F);

    const auto* roughness =
        draw.material.parameters.TryGet<woki::f32>(woki::gfx::material_parameters::kRoughness);
    REQUIRE(roughness != nullptr);
    REQUIRE(*roughness == 0.4F);
}
