cmake_minimum_required(VERSION 3.25)

project(gfx
    VERSION 1.0.0
    DESCRIPTION "graphics abstraction"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS
    include/*.hpp
    backend/*.hpp
)

file(GLOB_RECURSE SOURCES
    src/*.cpp
    backend/*.cpp
)

set(BASE_DEPS
    woki::core
)

if(EMSCRIPTEN)
    list(APPEND BASE_DEPS emdawnwebgpu)
else()
    list(APPEND BASE_DEPS glfw)

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

if(NOT EMSCRIPTEN)
    target_compile_definitions(woki_gfx PUBLIC GLFW_INCLUDE_NONE)
endif()
