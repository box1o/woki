#include "../../include/woki/log/log.hpp"

#ifdef __EMSCRIPTEN__

#include <emscripten/emscripten.h>

namespace slog {

void Configure(std::string, Level, const std::string&, const std::string&, std::size_t, std::size_t) {}

namespace detail {

void LogWebTrace(const char* msg) {
    emscripten_log(EM_LOG_CONSOLE | EM_LOG_DEBUG, "%s", msg);
}

void LogWebDebug(const char* msg) {
    emscripten_log(EM_LOG_CONSOLE | EM_LOG_DEBUG, "%s", msg);
}

void LogWebInfo(const char* msg) {
    emscripten_log(EM_LOG_CONSOLE | EM_LOG_INFO, "%s", msg);
}

void LogWebWarn(const char* msg) {
    emscripten_log(EM_LOG_CONSOLE | EM_LOG_WARN, "%s", msg);
}

void LogWebError(const char* msg) {
    emscripten_log(EM_LOG_CONSOLE | EM_LOG_ERROR, "%s", msg);
}

void LogWebCritical(const char* msg) {
    emscripten_log(EM_LOG_CONSOLE | EM_LOG_ERROR, "%s", msg);
}

} // namespace detail

} // namespace slog

#else

#include <memory>
#include <mutex>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace slog {

namespace detail {

std::shared_ptr<spdlog::logger>& LoggerRef() {
    static std::shared_ptr<spdlog::logger> logger;
    return logger;
}

std::mutex& LoggerMutex() {
    static std::mutex mutex;
    return mutex;
}

std::shared_ptr<spdlog::logger> Logger() noexcept {
    const std::scoped_lock lock(LoggerMutex());
    return LoggerRef();
}

} // namespace detail

void Configure(std::string name, Level level, const std::string& pattern, const std::string& logFile, std::size_t maxFileSize, std::size_t maxFiles) {
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;

    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    if (!logFile.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, maxFileSize, maxFiles));
    }

    auto logger = std::make_shared<spdlog::logger>(std::move(name), sinks.begin(), sinks.end());

    const auto spdlogLevel = [&] {
        using L = Level;
        using SL = spdlog::level::level_enum;

        switch (level) {
            case L::Trace:
                return SL::trace;
            case L::Debug:
                return SL::debug;
            case L::Info:
                return SL::info;
            case L::Warn:
                return SL::warn;
            case L::Error:
                return SL::err;
            case L::Critical:
                return SL::critical;
            case L::Off:
                return SL::off;
        }

        return SL::info;
    }();

    logger->set_level(spdlogLevel);
    logger->set_pattern(pattern);
    logger->flush_on(spdlog::level::warn);

    const std::scoped_lock lock(detail::LoggerMutex());
    detail::LoggerRef() = std::move(logger);
}

} // namespace slog

#endif
