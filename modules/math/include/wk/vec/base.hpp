#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <stdexcept>

#include "./fwd.hpp"
#include "../common/functions.hpp"
#include "../detail/arithmetic.hpp"

namespace wk::math {

template <std::size_t N, arithmetic T>
requires(N >= 2)
class vec {
public:
    static constexpr std::size_t size = N;
    using value_type = T;

    constexpr vec() noexcept = default;

    explicit constexpr vec(T scalar) noexcept {
        data_.fill(scalar);
    }

    template <arithmetic... Args>
    requires(sizeof...(Args) == N)
    constexpr vec(Args... args) noexcept
        : data_{static_cast<T>(args)...} {}

    template <arithmetic U>
    constexpr explicit vec(const vec<N, U>& other) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] = static_cast<T>(other[i]);
        }
    }

    [[nodiscard]] constexpr T& operator[](std::size_t i) noexcept {
        assert(i < N);
        return data_[i];
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t i) const noexcept {
        assert(i < N);
        return data_[i];
    }

    [[nodiscard]] constexpr T* data() noexcept {
        return data_.data();
    }

    [[nodiscard]] constexpr const T* data() const noexcept {
        return data_.data();
    }

    [[nodiscard]] constexpr T& x() noexcept
    requires(N >= 1)
    {
        return data_[0];
    }

    [[nodiscard]] constexpr const T& x() const noexcept
    requires(N >= 1)
    {
        return data_[0];
    }

    [[nodiscard]] constexpr T& y() noexcept
    requires(N >= 2)
    {
        return data_[1];
    }

    [[nodiscard]] constexpr const T& y() const noexcept
    requires(N >= 2)
    {
        return data_[1];
    }

    [[nodiscard]] constexpr T& z() noexcept
    requires(N >= 3)
    {
        return data_[2];
    }

    [[nodiscard]] constexpr const T& z() const noexcept
    requires(N >= 3)
    {
        return data_[2];
    }

    [[nodiscard]] constexpr T& w() noexcept
    requires(N >= 4)
    {
        return data_[3];
    }

    [[nodiscard]] constexpr const T& w() const noexcept
    requires(N >= 4)
    {
        return data_[3];
    }

    constexpr vec& operator+=(const vec& other) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] += other.data_[i];
        return *this;
    }

    constexpr vec& operator-=(const vec& other) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] -= other.data_[i];
        return *this;
    }

    constexpr vec& operator*=(const vec& other) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] *= other.data_[i];
        return *this;
    }

    constexpr vec& operator*=(T scalar) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] *= scalar;
        return *this;
    }

    constexpr vec& operator/=(const vec& other) {
        for (std::size_t i = 0; i < N; ++i) {
            if (other.data_[i] == T{}) {
                throw std::domain_error("vector division by zero");
            }
            data_[i] /= other.data_[i];
        }
        return *this;
    }

    constexpr vec& operator/=(T scalar) {
        if (scalar == T{}) {
            throw std::domain_error("vector division by zero");
        }
        for (std::size_t i = 0; i < N; ++i)
            data_[i] /= scalar;
        return *this;
    }

    [[nodiscard]] friend constexpr vec operator+(vec a, const vec& b) noexcept {
        a += b;
        return a;
    }

    [[nodiscard]] friend constexpr vec operator-(vec a, const vec& b) noexcept {
        a -= b;
        return a;
    }

    [[nodiscard]] friend constexpr vec operator*(vec a, const vec& b) noexcept {
        a *= b;
        return a;
    }

    [[nodiscard]] friend constexpr vec operator*(vec v, T s) noexcept {
        v *= s;
        return v;
    }

    [[nodiscard]] friend constexpr vec operator*(T s, vec v) noexcept {
        v *= s;
        return v;
    }

    [[nodiscard]] friend constexpr vec operator/(vec a, const vec& b) {
        a /= b;
        return a;
    }

    [[nodiscard]] friend constexpr vec operator/(vec v, T s) {
        v /= s;
        return v;
    }

    [[nodiscard]] constexpr vec operator-() const noexcept {
        vec result{};
        for (std::size_t i = 0; i < N; ++i)
            result.data_[i] = -data_[i];
        return result;
    }

    [[nodiscard]] friend constexpr bool operator==(const vec& a, const vec& b) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            if constexpr (floating_point<T>) {
                if (!approx_equal(a.data_[i], b.data_[i]))
                    return false;
            } else {
                if (a.data_[i] != b.data_[i])
                    return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool operator!=(const vec& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] constexpr T dot(const vec& other) const noexcept {
        T result{};
        for (std::size_t i = 0; i < N; ++i)
            result += data_[i] * other.data_[i];
        return result;
    }

    [[nodiscard]] constexpr T length_squared() const noexcept {
        return dot(*this);
    }

    [[nodiscard]] real_type<T> length() const noexcept {
        real_type<T> result{};
        for (const T value : data_) {
            result = std::hypot(result, static_cast<real_type<T>>(value));
        }
        return result;
    }

    [[nodiscard]] vec<N, real_type<T>> normalized() const noexcept {
        const real_type<T> l = length();
        vec<N, real_type<T>> result{};
        if (l == real_type<T>{}) {
            return result;
        }
        for (std::size_t i = 0; i < N; ++i) {
            result[i] = static_cast<real_type<T>>(data_[i]) / l;
        }
        return result;
    }

    constexpr vec& normalize() noexcept
    requires floating_point<T>
    {
        const T l = length();
        if (l != T{}) {
            *this /= l;
        }
        return *this;
    }

private:
    std::array<T, N> data_{};
};

} // namespace wk::math
