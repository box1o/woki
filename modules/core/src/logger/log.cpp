#include "../../include/wk/logger/log.hpp"

#include <mutex>
#include <utility>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <spdlog/sinks/callback_sink.h>
#else
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

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

#ifdef __EMSCRIPTEN__
    sinks.push_back(createRef<spdlog::sinks::callback_sink_mt>(
        [](const spdlog::details::log_msg& msg) {
            int flags = EM_LOG_CONSOLE;

            switch (msg.level) {
                case spdlog::level::trace:
                case spdlog::level::debug:
                    flags |= EM_LOG_DEBUG;
                    break;
                case spdlog::level::info:
                    flags |= EM_LOG_INFO;
                    break;
                case spdlog::level::warn:
                    flags |= EM_LOG_WARN;
                    break;
                case spdlog::level::err:
                case spdlog::level::critical:
                    flags |= EM_LOG_ERROR;
                    break;
                default:
                    break;
            }

            const std::string text(msg.payload.data(), msg.payload.size());
            emscripten_log(flags, "%s", text.c_str());
        }
    ));
#else
    sinks.push_back(createRef<spdlog::sinks::stdout_color_sink_mt>());

    if (!logFile.empty()) {
        sinks.push_back(createRef<spdlog::sinks::rotating_file_sink_mt>(logFile, maxFileSize, maxFiles));
    }
#endif

    auto logger = createRef<spdlog::logger>(std::move(name), sinks.begin(), sinks.end());

    logger->set_level(detail::to_spdlog_level(level));
    logger->set_pattern(pattern);
    logger->flush_on(spdlog::level::warn);

    auto& state = detail::_state();
    std::scoped_lock lock(state.mutex);
    state.logger = std::move(logger);
}

} // namespace wk::slog
