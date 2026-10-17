#pragma once

#include <cmath>
#include <stdexcept>

#include "op.hpp"
#include "../vec/base.hpp"
#include "../mat/base.hpp"

namespace wk::math {

template <floating_point T>
[[nodiscard]] inline vec<3, T> project_ndc(const vec<3, T>& world, const mat<4, 4, T>& view, const mat<4, 4, T>& proj) {
    const vec<4, T> point(world[0], world[1], world[2], T{1});
    const vec<4, T> clip = proj * (view * point);
    if (!std::isfinite(clip[3]) || clip[3] == T{}) {
        throw std::domain_error("projection requires a finite, nonzero homogeneous coordinate");
    }
    const vec<3, T> result(clip[0] / clip[3], clip[1] / clip[3], clip[2] / clip[3]);
    if (!std::isfinite(result[0]) || !std::isfinite(result[1]) || !std::isfinite(result[2])) {
        throw std::domain_error("projection produced non-finite coordinates");
    }
    return result;
}

template <floating_point T>
[[nodiscard]] inline vec<3, T> unproject_ndc(const vec<3, T>& ndc, const mat<4, 4, T>& view, const mat<4, 4, T>& proj) {
    const auto inverse_view_projection = inverse(proj * view);
    const vec<4, T> clip(ndc[0], ndc[1], ndc[2], T{1});
    const vec<4, T> world = inverse_view_projection * clip;
    if (!std::isfinite(world[3]) || world[3] == T{}) {
        throw std::domain_error("unprojection requires a finite, nonzero homogeneous coordinate");
    }
    const vec<3, T> result(world[0] / world[3], world[1] / world[3], world[2] / world[3]);
    if (!std::isfinite(result[0]) || !std::isfinite(result[1]) || !std::isfinite(result[2])) {
        throw std::domain_error("unprojection produced non-finite coordinates");
    }
    return result;
}

} // namespace wk::math
