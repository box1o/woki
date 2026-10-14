#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <spdlog/logger.h>

#include <wk/types.hpp>

namespace wk::slog {

enum class Level { Trace, Debug, Info, Warn, Error, Critical, Off };

void configure(
    std::string name = "woki",
    Level level = Level::Info,
    const std::string& logFile = "",
    std::size_t maxFileSize = 5 * 1024 * 1024,
    std::size_t maxFiles = 3,
    const std::string& pattern = "[%H:%M:%S] [%^%l%$] %v"
);

namespace detail {
ref<spdlog::logger> logger() noexcept;

template <typename... Args>
inline void log(spdlog::level::level_enum level, std::string_view format, Args&&... args) {
    if (auto l = logger(); l && l->should_log(level)) {
        auto message = fmt::vformat(format, fmt::make_format_args(args...));
        l->log(level, spdlog::string_view_t(message.data(), message.size()));
    }
}
} // namespace detail

// ================================================================================
// logger functions
// ================================================================================

template <typename... Args>
inline void trace(std::string_view format, Args&&... args) {
    detail::log(spdlog::level::trace, format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(std::string_view format, Args&&... args) {
    detail::log(spdlog::level::debug, format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(std::string_view format, Args&&... args) {
    detail::log(spdlog::level::info, format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warn(std::string_view format, Args&&... args) {
    detail::log(spdlog::level::warn, format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(std::string_view format, Args&&... args) {
    detail::log(spdlog::level::err, format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void critical(std::string_view format, Args&&... args) {
    detail::log(spdlog::level::critical, format, std::forward<Args>(args)...);
}

} // namespace wk::slog
