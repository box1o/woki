cmake_minimum_required(VERSION 3.25)

project(gui
    VERSION 1.0.0
    DESCRIPTION "core functionality"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS
    include/*.hpp
)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

set(BASE_DEPS
    woki::core
)

if (NOT EMSCRIPTEN)
    list(APPEND BASE_DEPS 
        fmt 
        spdlog
        glfw
    )
else()
    message(STATUS "Skipping fmt and spdlog for web build (using Emscripten's libc++)")
endif()


add_module(gui
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    DEPENDENCIES ${BASE_DEPS}
)
