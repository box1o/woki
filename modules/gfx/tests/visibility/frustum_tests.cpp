#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/visibility.hpp>

TEST_CASE("Identity frustum accepts bounds inside clip volume") {
    const auto frustum = woki::gfx::ExtractFrustum(woki::math::mat4f::identity());
    REQUIRE(frustum);
    REQUIRE(woki::gfx::Intersects(*frustum, {.center = {}, .radius = 0.5F}));
    REQUIRE_FALSE(woki::gfx::Intersects(*frustum, {.center = {3.0F, 0.0F, 0.0F}, .radius = 0.5F}));
}

TEST_CASE("Bounding spheres follow object translation and maximum scale") {
    const woki::gfx::BoundingSphere bounds{.center = {1.0F, 0.0F, 0.0F}, .radius = 2.0F};
    const auto transform = woki::math::translate(woki::math::vec3f{3.0F, 0.0F, 0.0F}) *
                           woki::math::scale(woki::math::vec3f{2.0F, 3.0F, 4.0F});

    const auto transformed = woki::gfx::TransformBounds(bounds, transform);
    REQUIRE(transformed.center.x == 5.0F);
    REQUIRE(transformed.radius == 8.0F);
}

TEST_CASE("Bounding sphere validation rejects negative radius") {
    REQUIRE_FALSE(woki::gfx::Validate(woki::gfx::BoundingSphere{.radius = -1.0F}));
}
