#pragma once

#include <cmath>
#include <cstddef>
#include <format>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include "../mat/base.hpp"
#include "../vec/base.hpp"

namespace wk::math {
namespace detail {

template <arithmetic T>
inline void format_scalar(std::ostream& stream, T value) {
    if constexpr (std::is_floating_point_v<T>) {
        std::ostringstream temp;
        temp << std::fixed << std::setprecision(2) << value;

        auto text = temp.str();
        if (auto pos = text.find('.'); pos != std::string::npos) {
            text.erase(text.find_last_not_of('0') + 1);
            if (text.back() == '.')
                text.pop_back();
        }

        stream << text;
    } else {
        stream << value;
    }
}

template <std::size_t N, arithmetic T>
[[nodiscard]] inline std::string format_value(const vec<N, T>& value) {
    std::ostringstream stream;
    stream << "vec" << N << "(";

    for (std::size_t i = 0; i < N; ++i) {
        if (i)
            stream << ", ";
        format_scalar(stream, value[i]);
    }

    stream << ')';
    return stream.str();
}

template <std::size_t Rows, std::size_t Cols, arithmetic T>
[[nodiscard]] inline std::string format_value(const mat<Rows, Cols, T>& value) {
    std::ostringstream stream;
    stream << "mat" << Rows << "x" << Cols << "(\n";

    for (std::size_t row = 0; row < Rows; ++row) {
        stream << "  [";

        for (std::size_t col = 0; col < Cols; ++col) {
            if (col)
                stream << '\t';
            format_scalar(stream, value(row, col));
        }

        stream << ']';
        if (row + 1 < Rows)
            stream << ',';
        stream << '\n';
    }

    stream << ')';
    return stream.str();
}

} // namespace detail

template <std::size_t N, arithmetic T>
[[nodiscard]] inline std::string to_string(const vec<N, T>& value) {
    return detail::format_value(value);
}

template <std::size_t Rows, std::size_t Cols, arithmetic T>
[[nodiscard]] inline std::string to_string(const mat<Rows, Cols, T>& value) {
    return detail::format_value(value);
}

template <std::size_t N, arithmetic T>
inline std::ostream& operator<<(std::ostream& stream, const vec<N, T>& value) {
    return stream << to_string(value);
}

template <std::size_t Rows, std::size_t Cols, arithmetic T>
inline std::ostream& operator<<(std::ostream& stream, const mat<Rows, Cols, T>& value) {
    return stream << to_string(value);
}

} // namespace wk::math

namespace std {

template <std::size_t N, wk::math::arithmetic T>
struct formatter<wk::math::vec<N, T>> : formatter<string_view> {
    using base = formatter<string_view>;

    template <typename FormatContext>
    auto format(const wk::math::vec<N, T>& value, FormatContext& context) const {
        const auto text = wk::math::to_string(value);
        return base::format(string_view(text), context);
    }
};

template <std::size_t Rows, std::size_t Cols, wk::math::arithmetic T>
struct formatter<wk::math::mat<Rows, Cols, T>> : formatter<string_view> {
    using base = formatter<string_view>;

    template <typename FormatContext>
    auto format(const wk::math::mat<Rows, Cols, T>& value, FormatContext& context) const {
        const auto text = wk::math::to_string(value);
        return base::format(string_view(text), context);
    }
};

} // namespace std

#if __has_include(<fmt/format.h>)
#include <fmt/format.h>

template <std::size_t N, wk::math::arithmetic T>
struct fmt::formatter<wk::math::vec<N, T>> : fmt::formatter<fmt::string_view> {
    template <typename FormatContext>
    auto format(const wk::math::vec<N, T>& value, FormatContext& context) const {
        const auto text = wk::math::to_string(value);
        return fmt::formatter<fmt::string_view>::format(fmt::string_view(text), context);
    }
};

template <std::size_t Rows, std::size_t Cols, wk::math::arithmetic T>
struct fmt::formatter<wk::math::mat<Rows, Cols, T>> : fmt::formatter<fmt::string_view> {
    template <typename FormatContext>
    auto format(const wk::math::mat<Rows, Cols, T>& value, FormatContext& context) const {
        const auto text = wk::math::to_string(value);
        return fmt::formatter<fmt::string_view>::format(fmt::string_view(text), context);
    }
};
#endif
