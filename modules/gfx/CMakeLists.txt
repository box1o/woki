cmake_minimum_required(VERSION 3.25)

set(_GFX_LANGUAGES CXX)
if(APPLE)
    list(APPEND _GFX_LANGUAGES OBJCXX)
endif()

project(gfx
    VERSION 1.0.0
    DESCRIPTION "graphics abstraction"
    LANGUAGES ${_GFX_LANGUAGES}
)

file(GLOB_RECURSE HEADERS
    include/*.hpp
    backend/*.hpp
)

file(GLOB_RECURSE SOURCES
    src/*.cpp
    backend/*.cpp
)

if(APPLE)
    file(GLOB_RECURSE APPLE_SOURCES
        backend/*.mm
    )
    list(APPEND SOURCES ${APPLE_SOURCES})
endif()

set(BASE_DEPS
    woki::core
)

if(EMSCRIPTEN)
    if(NOT DEFINED EMDAWNWEBGPU_PORT OR EMDAWNWEBGPU_PORT STREQUAL "")
        set(_WGPU_PORT emdawnwebgpu)
    else()
        set(_WGPU_PORT "${EMDAWNWEBGPU_PORT}")
    endif()
else()
    find_package(Threads REQUIRED)
    find_package(Dawn CONFIG QUIET)

    list(APPEND BASE_DEPS glfw)

    if(APPLE)
        list(APPEND BASE_DEPS
            "-framework Foundation"
            "-framework QuartzCore"
            "-framework AppKit"
        )
    endif()

    if(TARGET dawn::webgpu_dawn)
        list(APPEND BASE_DEPS dawn::webgpu_dawn)
    elseif(TARGET webgpu_dawn)
        list(APPEND BASE_DEPS webgpu_dawn)
    endif()
endif()

add_module(gfx
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    DEPENDENCIES ${BASE_DEPS}
)

if(EMSCRIPTEN)
    target_compile_options(woki_gfx PUBLIC
        "--use-port=${_WGPU_PORT}"
    )
else()
    target_compile_definitions(woki_gfx PUBLIC GLFW_INCLUDE_NONE)
endif()
