project(core
    VERSION 1.0.0
    DESCRIPTION "core shared functionality"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

set(_DEPS)
set(_PDEPS)

list(APPEND _DEPS
    fmt::fmt
    spdlog::spdlog
)

list(APPEND _PDEPS
)

add_module(core
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    PUBLIC_DEPENDENCIES ${_DEPS}
    PRIVATE_DEPENDENCIES ${_PDEPS}
)

install(
    DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wk/docs/core
    FILES_MATCHING PATTERN "*.md"
)

# if(BUILD_TESTING)
#     add_subdirectory(tests)
# endif()
