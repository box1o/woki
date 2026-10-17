# ================================================================================
# Shared dependencies
# ================================================================================

find_or_add(fmt
    NO_SYSTEM
    GIT_TAG 12.2.1
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)

find_or_add(spdlog
    NO_SYSTEM
    GIT_TAG v1.15.3
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    CMAKE_CACHE_VARS
        "SPDLOG_FMT_EXTERNAL=ON"
)

find_or_add(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
)
