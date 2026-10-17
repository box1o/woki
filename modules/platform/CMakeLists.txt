project(platform
    VERSION 1.0.0
    DESCRIPTION "platform functionality"
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
    wk::core
    wk::math
)

list(APPEND _PDEPS
)

add_module(${PROJECT_NAME}
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    PUBLIC_DEPENDENCIES ${_DEPS}
    PRIVATE_DEPENDENCIES ${_PDEPS}
)

install(
    DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wk/docs/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.md"
)

# if(BUILD_TESTING)
#     add_subdirectory(tests)
# endif()
