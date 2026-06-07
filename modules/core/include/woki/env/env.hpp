#pragma once

// IWYU pragma: private, include "woki/core.hpp"

#include "../error/result.hpp"

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace woki::env {

[[nodiscard]] inline bool Has(std::string_view name) {
    const char* value = std::getenv(std::string(name).c_str());
    return value != nullptr;
}

[[nodiscard]] inline std::optional<std::string> TryGet(std::string_view name) {
    const char* value = std::getenv(std::string(name).c_str());
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
}

[[nodiscard]] inline Result<std::string> Get(std::string_view name) {
    auto value = TryGet(name);
    if (!value.has_value()) {
        return Err(ErrorCode::FileNotFound, "Environment variable not found");
    }
    return Ok(std::move(*value));
}

[[nodiscard]] inline std::string GetOr(std::string_view name, std::string default_value) {
    auto value = TryGet(name);
    if (!value.has_value()) {
        return default_value;
    }
    return *value;
}

} // namespace woki::env
