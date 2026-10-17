#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <spdlog/logger.h>

#include <wk/core/types.hpp>

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

[[nodiscard]] constexpr spdlog::level::level_enum to_spdlog_level(Level level) noexcept {
    switch (level) {
        case Level::Trace:
            return spdlog::level::trace;
        case Level::Debug:
            return spdlog::level::debug;
        case Level::Info:
            return spdlog::level::info;
        case Level::Warn:
            return spdlog::level::warn;
        case Level::Error:
            return spdlog::level::err;
        case Level::Critical:
            return spdlog::level::critical;
        case Level::Off:
            return spdlog::level::off;
    }
    return spdlog::level::off;
}

template <typename... Args>
inline void log(spdlog::level::level_enum level, std::string_view format, Args&&... args) {
    if (auto l = logger(); l && l->should_log(level)) {
        try {
            const auto message = fmt::vformat(format, fmt::make_format_args(args...));
            l->log(level, spdlog::string_view_t(message.data(), message.size()));
        } catch (const fmt::format_error&) {
            l->log(level, spdlog::string_view_t(format.data(), format.size()));
        }
    }
}
} // namespace detail

// ================================================================================
// logger functions
// ================================================================================

template <typename... Args>
inline void trace(std::string_view format, Args&&... args) {
    detail::log(detail::to_spdlog_level(Level::Trace), format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(std::string_view format, Args&&... args) {
    detail::log(detail::to_spdlog_level(Level::Debug), format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(std::string_view format, Args&&... args) {
    detail::log(detail::to_spdlog_level(Level::Info), format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warn(std::string_view format, Args&&... args) {
    detail::log(detail::to_spdlog_level(Level::Warn), format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(std::string_view format, Args&&... args) {
    detail::log(detail::to_spdlog_level(Level::Error), format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void critical(std::string_view format, Args&&... args) {
    detail::log(detail::to_spdlog_level(Level::Critical), format, std::forward<Args>(args)...);
}

} // namespace wk::slog
