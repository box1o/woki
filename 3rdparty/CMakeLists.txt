# ================================================================================
# Shared dependencies
# ================================================================================

find_or_add(fmt
    NO_SYSTEM
    GIT_TAG main
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)
target_compile_definitions(fmt PUBLIC FMT_CONSTEVAL=)

find_or_add(spdlog
    NO_SYSTEM
    GIT_TAG v1.15.3
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    CMAKE_CACHE_VARS
        "SPDLOG_FMT_EXTERNAL=ON"
)
