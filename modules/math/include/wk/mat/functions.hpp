#pragma once

#include "base.hpp"

namespace wk::math {

template <std::size_t Rows, std::size_t Cols, arithmetic T>
[[nodiscard]] constexpr mat<Cols, Rows, T> transpose(const mat<Rows, Cols, T>& m) noexcept {
    return m.transpose();
}

template <std::size_t N, arithmetic T>
[[nodiscard]] auto determinant(const mat<N, N, T>& m) noexcept {
    return det(m);
}

template <std::size_t N, arithmetic T>
[[nodiscard]] constexpr T trace(const mat<N, N, T>& m) noexcept {
    T result{};
    for (std::size_t i = 0; i < N; ++i) {
        result += m(i, i);
    }
    return result;
}

} // namespace wk::math
