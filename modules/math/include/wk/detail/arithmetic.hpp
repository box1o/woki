#pragma once

#include <cstdint>
#include <concepts>
#include <type_traits>

namespace wk::math {

template <typename T>
concept integral = std::integral<T>;

template <typename T>
concept floating_point = std::floating_point<T>;

template <typename T>
concept arithmetic = integral<T> || floating_point<T>;

template <typename T>
concept signed_arithmetic = std::signed_integral<T> || floating_point<T>;

template <typename T>
concept unsigned_arithmetic = arithmetic<T> && std::unsigned_integral<T>;

template <arithmetic T>
using real_type = std::conditional_t<floating_point<T>, T, double>;

template <arithmetic T>
struct magnitude {
    using type = T;
};

template <std::signed_integral T>
struct magnitude<T> {
    using type = std::make_unsigned_t<T>;
};

template <arithmetic T>
using magnitude_type = typename magnitude<T>::type;

enum class layout : std::uint16_t { rowm, colm };

} // namespace wk::math
