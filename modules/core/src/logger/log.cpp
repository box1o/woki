#include "../../include/wk/logger/log.hpp"

#include <mutex>
#include <utility>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace wk::slog::detail {

struct LoggerState {
    std::mutex mutex;
    ref<spdlog::logger> logger;
};

inline LoggerState& _state() {
    static LoggerState state;
    return state;
}

ref<spdlog::logger> logger() noexcept {
    auto& state = _state();
    std::scoped_lock lock(state.mutex);
    return state.logger;
}

} // namespace wk::slog::detail

namespace wk::slog {

void configure(
    std::string name,
    Level level,
    const std::string& logFile,
    std::size_t maxFileSize,
    std::size_t maxFiles,
    const std::string& pattern
) {
    std::vector<spdlog::sink_ptr> sinks;

    sinks.push_back(createRef<spdlog::sinks::stdout_color_sink_mt>());

    if (!logFile.empty()) {
        sinks.push_back(createRef<spdlog::sinks::rotating_file_sink_mt>(logFile, maxFileSize, maxFiles));
    }

    auto logger = createRef<spdlog::logger>(std::move(name), sinks.begin(), sinks.end());

    constexpr spdlog::level::level_enum levels[] = {spdlog::level::trace,
        spdlog::level::debug,
        spdlog::level::info,
        spdlog::level::warn,
        spdlog::level::err,
        spdlog::level::critical,
        spdlog::level::off};

    logger->set_level(levels[static_cast<std::size_t>(level)]);
    logger->set_pattern(pattern);
    logger->flush_on(spdlog::level::warn);

    auto& state = detail::_state();
    std::scoped_lock lock(state.mutex);
    state.logger = std::move(logger);
}

} // namespace wk::slog
