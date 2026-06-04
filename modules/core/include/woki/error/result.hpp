#pragma once

// IWYU pragma: private, include "woki/core.hpp"

#include "errors.hpp"

#include <expected>
#include <source_location>
#include <type_traits>
#include <utility>

namespace woki {

template <typename T>
using Result = std::expected<T, Error>;

template <typename T>
    requires(!std::same_as<std::decay_t<T>, std::unexpected<Error>>)
[[nodiscard]] constexpr auto Ok(T&& value) -> Result<std::decay_t<T>> {
    return Result<std::decay_t<T>>(std::forward<T>(value));
}

[[nodiscard]] constexpr auto Ok() noexcept -> Result<void> {
    return Result<void>();
}

[[nodiscard]] inline auto MakeError(
    ErrorCode code,
    std::string_view message = {},
    std::source_location location = std::source_location::current()) -> Error {
    return Error(code, message, location);
}

[[nodiscard]] inline auto Err(
    ErrorCode code,
    std::string_view message,
    std::source_location location = std::source_location::current()) -> std::unexpected<Error> {
    return std::unexpected<Error>(MakeError(code, message, location));
}

[[nodiscard]] inline auto Err(
    ErrorCode code,
    std::source_location location = std::source_location::current()) -> std::unexpected<Error> {
    return std::unexpected<Error>(MakeError(code, {}, location));
}

[[nodiscard]] inline auto Err(Error error) -> std::unexpected<Error> {
    return std::unexpected<Error>(std::move(error));
}

} // namespace woki
