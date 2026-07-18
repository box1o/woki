#include <woki/gfx/visibility/frustum.hpp>

#include <algorithm>
#include <cmath>

namespace woki::gfx {
namespace {

[[nodiscard]] bool IsFinite(const math::vec3f value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] Result<Plane> Normalize(const math::vec4f value) {
    const math::vec3f normal{value.x, value.y, value.z};
    const f32 length = normal.length();
    if (!std::isfinite(length) || length <= 0.0F || !std::isfinite(value.w)) {
        return Err(ErrorCode::ValidationInvalidState,
            "View projection matrix does not produce a valid frustum");
    }
    return Ok(Plane{.normal = normal / length, .distance = value.w / length});
}

} // namespace

Result<void> Validate(const BoundingSphere& bounds) {
    if (!IsFinite(bounds.center) || !std::isfinite(bounds.radius) || bounds.radius < 0.0F) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Bounding sphere requires a finite center and nonnegative radius");
    }
    return Ok();
}

Result<Frustum> ExtractFrustum(const math::mat4f& matrix) {
    const math::vec4f row0{matrix(0, 0), matrix(0, 1), matrix(0, 2), matrix(0, 3)};
    const math::vec4f row1{matrix(1, 0), matrix(1, 1), matrix(1, 2), matrix(1, 3)};
    const math::vec4f row2{matrix(2, 0), matrix(2, 1), matrix(2, 2), matrix(2, 3)};
    const math::vec4f row3{matrix(3, 0), matrix(3, 1), matrix(3, 2), matrix(3, 3)};
    const std::array<math::vec4f, 6> candidates{
        row3 + row0, row3 - row0, row3 + row1, row3 - row1, row3 + row2, row3 - row2};
    Frustum frustum{};
    for (u32 index = 0; index < candidates.size(); ++index) {
        auto plane = Normalize(candidates[index]);
        if (!plane) {
            return Err(plane.error());
        }
        frustum.planes[index] = *plane;
    }
    return Ok(frustum);
}

BoundingSphere TransformBounds(
    const BoundingSphere& bounds, const math::mat4f& transform) noexcept {
    const math::vec4f center =
        transform * math::vec4f{bounds.center.x, bounds.center.y, bounds.center.z, 1.0F};
    const f32 scale_x = math::vec3f{transform(0, 0), transform(1, 0), transform(2, 0)}.length();
    const f32 scale_y = math::vec3f{transform(0, 1), transform(1, 1), transform(2, 1)}.length();
    const f32 scale_z = math::vec3f{transform(0, 2), transform(1, 2), transform(2, 2)}.length();
    return {
        .center = {center.x, center.y, center.z},
        .radius = bounds.radius * std::max({scale_x, scale_y, scale_z}),
    };
}

bool Intersects(const Frustum& frustum, const BoundingSphere& bounds) noexcept {
    return std::ranges::all_of(frustum.planes, [&bounds](const Plane& plane) {
        return plane.normal.dot(bounds.center) + plane.distance >= -bounds.radius;
    });
}

} // namespace woki::gfx
