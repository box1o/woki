#pragma once

#include <array>

#include <woki/core.hpp>
#include <woki/math/math.hpp>

namespace woki::gfx {

struct Plane final {
    math::vec3f normal{};
    f32 distance{0.0F};
};

struct BoundingSphere final {
    math::vec3f center{};
    f32 radius{0.0F};
};

struct Frustum final {
    std::array<Plane, 6> planes{};
};

[[nodiscard]] Result<void> Validate(const BoundingSphere& bounds);
[[nodiscard]] Result<Frustum> ExtractFrustum(const math::mat4f& view_projection);
[[nodiscard]] BoundingSphere TransformBounds(
    const BoundingSphere& bounds, const math::mat4f& transform) noexcept;
[[nodiscard]] bool Intersects(const Frustum& frustum, const BoundingSphere& bounds) noexcept;

} // namespace woki::gfx
