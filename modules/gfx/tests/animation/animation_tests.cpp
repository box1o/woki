#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/animations.hpp>

TEST_CASE("Animation evaluates interpolated hierarchical poses") {
    const woki::gfx::Skeleton skeleton{.joints = {
                                           {.name = "root"},
                                           {.name = "child", .parent = 0},
                                       }};
    const woki::gfx::AnimationClip clip{
        .name = "move",
        .duration = 2.0F,
        .tracks = {{
            .joint = 0,
            .translations = {{.time = 0.0F, .value = {0.0F, 0.0F, 0.0F}},
                {.time = 2.0F, .value = {2.0F, 0.0F, 0.0F}}},
        }},
    };

    const auto pose =
        woki::gfx::Evaluate(skeleton, clip, 1.0F, woki::gfx::AnimationWrapMode::Clamp);
    REQUIRE(pose);
    REQUIRE(pose->local[0].translation.x == 1.0F);
    REQUIRE(pose->global.size() == 2);
    REQUIRE(pose->skin_matrices.size() == 2);
}

TEST_CASE("Animation loop mode wraps time") {
    const woki::gfx::Skeleton skeleton{.joints = {{.name = "root"}}};
    const woki::gfx::AnimationClip clip{
        .duration = 1.0F,
        .tracks = {{
            .joint = 0,
            .translations = {{.time = 0.0F, .value = {0.0F, 0.0F, 0.0F}},
                {.time = 1.0F, .value = {1.0F, 0.0F, 0.0F}}},
        }},
    };

    const auto pose = woki::gfx::Evaluate(skeleton, clip, 1.25F);
    REQUIRE(pose);
    REQUIRE(pose->local[0].translation.x == 0.25F);
}

TEST_CASE("Animation rejects invalid skeleton hierarchy and key order") {
    const woki::gfx::Skeleton invalid_skeleton{.joints = {{.name = "root", .parent = 0}}};
    REQUIRE_FALSE(woki::gfx::Validate(invalid_skeleton));

    const woki::gfx::AnimationClip invalid_clip{
        .duration = 1.0F,
        .tracks = {{.joint = 0, .translations = {{.time = 0.5F}, {.time = 0.25F}}}},
    };
    REQUIRE_FALSE(woki::gfx::Validate(invalid_clip, 1));
}
