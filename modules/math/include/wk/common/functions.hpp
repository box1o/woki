#pragma once

#include <cmath>
#include <limits>

#include "./constants.hpp"
#include "../detail/arithmetic.hpp"

namespace wk::math {

template <floating_point T>
[[nodiscard]] constexpr T radians(T degrees) noexcept {
    return degrees * (pi<T> / T{180});
}

template <floating_point T>
[[nodiscard]] constexpr T degrees(T radians_value) noexcept {
    return radians_value * (T{180} / pi<T>);
}

template <floating_point T>
[[nodiscard]] constexpr T lerp(T a, T b, T t) noexcept {
    return std::lerp(a, b, t);
}

template <arithmetic T>
[[nodiscard]] constexpr T sign(T value) noexcept {
    if constexpr (unsigned_arithmetic<T>) {
        return value == T{0} ? T{0} : T{1};
    } else {
        if (value > T{0})
            return T{1};
        if (value < T{0})
            return T{-1};
        return T{0};
    }
}

template <floating_point T>
[[nodiscard]] constexpr T abs(T value) noexcept {
    return std::fabs(value);
}

template <unsigned_arithmetic T>
[[nodiscard]] constexpr T abs(T value) noexcept {
    return value;
}

template <std::signed_integral T>
[[nodiscard]] constexpr magnitude_type<T> abs(T value) noexcept {
    using U = magnitude_type<T>;
    return value < T{} ? U{} - static_cast<U>(value) : static_cast<U>(value);
}

template <arithmetic T>
[[nodiscard]] constexpr T min(T a, T b) noexcept {
    return a < b ? a : b;
}

template <arithmetic T>
[[nodiscard]] constexpr T max(T a, T b) noexcept {
    return a > b ? a : b;
}

template <arithmetic T>
[[nodiscard]] constexpr T clamp(T value, T lo, T hi) noexcept {
    return min(max(value, lo), hi);
}

template <floating_point T>
[[nodiscard]] inline T sqrt(T value) noexcept {
    return std::sqrt(value);
}

template <integral T>
[[nodiscard]] inline double sqrt(T value) noexcept {
    return std::sqrt(static_cast<double>(value));
}

template <floating_point T>
[[nodiscard]] constexpr bool approx_equal(T a, T b, T tolerance = epsilon<T> * T{4}) noexcept {
    if (a == b) {
        return true;
    }
    if (std::isnan(a) || std::isnan(b) || tolerance < T{}) {
        return false;
    }
    return abs(a - b) <= tolerance * max(T{1}, max(abs(a), abs(b)));
}

using std::acos;
using std::asin;
using std::atan;
using std::atan2;
using std::ceil;
using std::cos;
using std::exp;
using std::floor;
using std::pow;
using std::round;
using std::sin;
using std::tan;
using std::trunc;

} // namespace wk::math
