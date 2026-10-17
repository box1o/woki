#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>

#include "../common/functions.hpp"
#include "../detail/arithmetic.hpp"
#include "fwd.hpp"

namespace wk::math {

template <arithmetic T>
using determinant_type = std::conditional_t<integral<T>, long double, T>;

template <std::size_t N, arithmetic T>
[[nodiscard]] determinant_type<T> det(const mat<N, N, T>& matrix) noexcept;

// Matrices are column-major: element (row, column) lives at data_[column * Rows + row].
template <std::size_t Rows, std::size_t Cols, arithmetic T>
class mat {
    static_assert(Rows > 0 && Cols > 0, "matrix dimensions must be positive");

public:
    static constexpr std::size_t rows = Rows;
    static constexpr std::size_t cols = Cols;
    using value_type = T;
    using col_type = std::array<T, Rows>;
    using row_type = std::array<T, Cols>;
    using column_view = std::span<T, Rows>;
    using const_column_view = std::span<const T, Rows>;

    constexpr mat() noexcept = default;

    explicit constexpr mat(T value) noexcept {
        data_.fill(value);
    }

    template <arithmetic... Args>
    requires(sizeof...(Args) == Rows * Cols)
    constexpr mat(layout order, Args... args) noexcept {
        const std::array values{static_cast<T>(args)...};
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                (*this)(row, col) = values[order == layout::rowm ? row * Cols + col : col * Rows + row];
            }
        }
    }

    template <arithmetic U>
    constexpr explicit mat(const mat<Rows, Cols, U>& other) noexcept {
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                (*this)(row, col) = static_cast<T>(other(row, col));
            }
        }
    }

    [[nodiscard]] static constexpr mat identity() noexcept
    requires(Rows == Cols)
    {
        mat result{};
        for (std::size_t i = 0; i < Rows; ++i) {
            result(i, i) = T{1};
        }
        return result;
    }

    [[nodiscard]] constexpr T& operator()(std::size_t row, std::size_t col) noexcept {
        assert(row < Rows && col < Cols);
        return data_[index(row, col)];
    }

    [[nodiscard]] constexpr const T& operator()(std::size_t row, std::size_t col) const noexcept {
        assert(row < Rows && col < Cols);
        return data_[index(row, col)];
    }

    [[nodiscard]] constexpr column_view operator[](std::size_t col) noexcept {
        return column(col);
    }

    [[nodiscard]] constexpr const_column_view operator[](std::size_t col) const noexcept {
        return column(col);
    }

    [[nodiscard]] constexpr column_view column(std::size_t col) noexcept {
        assert(col < Cols);
        return column_view(data_.data() + col * Rows, Rows);
    }

    [[nodiscard]] constexpr const_column_view column(std::size_t col) const noexcept {
        assert(col < Cols);
        return const_column_view(data_.data() + col * Rows, Rows);
    }

    constexpr void set_column(std::size_t col, const col_type& values) noexcept {
        assert(col < Cols);
        for (std::size_t row = 0; row < Rows; ++row) {
            (*this)(row, col) = values[row];
        }
    }

    [[nodiscard]] constexpr row_type row(std::size_t row) const noexcept {
        assert(row < Rows);
        row_type values{};
        for (std::size_t col = 0; col < Cols; ++col) {
            values[col] = (*this)(row, col);
        }
        return values;
    }

    constexpr void set_row(std::size_t row, const row_type& values) noexcept {
        assert(row < Rows);
        for (std::size_t col = 0; col < Cols; ++col) {
            (*this)(row, col) = values[col];
        }
    }

    [[nodiscard]] constexpr T* data() noexcept {
        return data_.data();
    }

    [[nodiscard]] constexpr const T* data() const noexcept {
        return data_.data();
    }

    constexpr mat& operator+=(const mat& rhs) noexcept {
        for (std::size_t i = 0; i < data_.size(); ++i) {
            data_[i] += rhs.data_[i];
        }
        return *this;
    }

    constexpr mat& operator-=(const mat& rhs) noexcept {
        for (std::size_t i = 0; i < data_.size(); ++i) {
            data_[i] -= rhs.data_[i];
        }
        return *this;
    }

    constexpr mat& operator*=(T scalar) noexcept {
        for (T& value : data_) {
            value *= scalar;
        }
        return *this;
    }

    constexpr mat& operator/=(T scalar) {
        if (scalar == T{}) {
            throw std::domain_error("matrix division by zero");
        }
        for (T& value : data_) {
            value /= scalar;
        }
        return *this;
    }

    [[nodiscard]] friend constexpr mat operator+(mat lhs, const mat& rhs) noexcept {
        return lhs += rhs;
    }

    [[nodiscard]] friend constexpr mat operator-(mat lhs, const mat& rhs) noexcept {
        return lhs -= rhs;
    }

    [[nodiscard]] friend constexpr mat operator*(mat matrix, T scalar) noexcept {
        return matrix *= scalar;
    }

    [[nodiscard]] friend constexpr mat operator*(T scalar, mat matrix) noexcept {
        return matrix *= scalar;
    }

    [[nodiscard]] friend constexpr mat operator/(mat matrix, T scalar) {
        return matrix /= scalar;
    }

    [[nodiscard]] constexpr mat<Cols, Rows, T> transpose() const noexcept {
        mat<Cols, Rows, T> result{};
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                result(col, row) = (*this)(row, col);
            }
        }
        return result;
    }

    [[nodiscard]] auto det() const noexcept
    requires(Rows == Cols)
    {
        return wk::math::det(*this);
    }

    [[nodiscard]] mat inverse() const
    requires(Rows == Cols && floating_point<T>);

    [[nodiscard]] friend constexpr bool operator==(const mat& lhs, const mat& rhs) noexcept {
        for (std::size_t i = 0; i < lhs.data_.size(); ++i) {
            if constexpr (floating_point<T>) {
                if (!approx_equal(lhs.data_[i], rhs.data_[i])) {
                    return false;
                }
            } else if (lhs.data_[i] != rhs.data_[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool operator!=(const mat& other) const noexcept {
        return !(*this == other);
    }

private:
    [[nodiscard]] static constexpr std::size_t index(std::size_t row, std::size_t col) noexcept {
        return col * Rows + row;
    }

    std::array<T, Rows * Cols> data_{};
};

template <std::size_t R, std::size_t C, std::size_t K, arithmetic T>
[[nodiscard]] constexpr mat<R, K, T> operator*(const mat<R, C, T>& lhs, const mat<C, K, T>& rhs) noexcept {
    mat<R, K, T> result{};
    for (std::size_t row = 0; row < R; ++row) {
        for (std::size_t col = 0; col < K; ++col) {
            for (std::size_t inner = 0; inner < C; ++inner) {
                result(row, col) += lhs(row, inner) * rhs(inner, col);
            }
        }
    }
    return result;
}

template <std::size_t N, arithmetic T>
[[nodiscard]] inline determinant_type<T> det(const mat<N, N, T>& matrix) noexcept {
    using R = determinant_type<T>;
    std::array<R, N * N> values{};
    const auto at = [&values](std::size_t row, std::size_t col) -> R& { return values[row * N + col]; };

    for (std::size_t row = 0; row < N; ++row) {
        for (std::size_t col = 0; col < N; ++col) {
            at(row, col) = static_cast<R>(matrix(row, col));
        }
    }

    if constexpr (N == 1) {
        return at(0, 0);
    }

    R previous = R{1};
    int sign = 1;
    for (std::size_t pivot_col = 0; pivot_col + 1 < N; ++pivot_col) {
        std::size_t pivot_row = pivot_col;
        for (std::size_t row = pivot_col + 1; row < N; ++row) {
            if (std::abs(at(row, pivot_col)) > std::abs(at(pivot_row, pivot_col))) {
                pivot_row = row;
            }
        }
        if (at(pivot_row, pivot_col) == R{}) {
            return R{};
        }
        if (pivot_row != pivot_col) {
            for (std::size_t col = pivot_col; col < N; ++col) {
                std::swap(at(pivot_row, col), at(pivot_col, col));
            }
            sign = -sign;
        }

        const R pivot = at(pivot_col, pivot_col);
        for (std::size_t row = pivot_col + 1; row < N; ++row) {
            for (std::size_t col = pivot_col + 1; col < N; ++col) {
                at(row, col) = (at(row, col) * pivot - at(row, pivot_col) * at(pivot_col, col)) / previous;
            }
        }
        for (std::size_t row = pivot_col + 1; row < N; ++row) {
            at(row, pivot_col) = R{};
        }
        previous = pivot;
    }
    return sign > 0 ? at(N - 1, N - 1) : -at(N - 1, N - 1);
}

template <std::size_t N, floating_point T>
[[nodiscard]] mat<N, N, T> inverse(const mat<N, N, T>& matrix) {
    mat<N, N, T> left(matrix);
    mat<N, N, T> right = mat<N, N, T>::identity();

    for (std::size_t pivot_col = 0; pivot_col < N; ++pivot_col) {
        std::size_t pivot_row = pivot_col;
        for (std::size_t row = pivot_col + 1; row < N; ++row) {
            if (std::abs(left(row, pivot_col)) > std::abs(left(pivot_row, pivot_col))) {
                pivot_row = row;
            }
        }
        const T pivot = left(pivot_row, pivot_col);
        if (!std::isfinite(pivot) || pivot == T{}) {
            throw std::domain_error("cannot invert a singular or non-finite matrix");
        }
        if (pivot_row != pivot_col) {
            for (std::size_t col = 0; col < N; ++col) {
                std::swap(left(pivot_row, col), left(pivot_col, col));
                std::swap(right(pivot_row, col), right(pivot_col, col));
            }
        }

        for (std::size_t col = 0; col < N; ++col) {
            left(pivot_col, col) /= pivot;
            right(pivot_col, col) /= pivot;
        }
        for (std::size_t row = 0; row < N; ++row) {
            if (row == pivot_col) {
                continue;
            }
            const T factor = left(row, pivot_col);
            for (std::size_t col = 0; col < N; ++col) {
                left(row, col) -= factor * left(pivot_col, col);
                right(row, col) -= factor * right(pivot_col, col);
            }
        }
    }
    return right;
}

template <std::size_t Rows, std::size_t Cols, arithmetic T>
mat<Rows, Cols, T> mat<Rows, Cols, T>::inverse() const
requires(Rows == Cols && floating_point<T>)
{
    return wk::math::inverse(*this);
}

} // namespace wk::math
