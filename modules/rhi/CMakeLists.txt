cmake_minimum_required(VERSION 3.25)


set(_RHI_LANGUAGES CXX)
if(APPLE)
    list(APPEND _RHI_LANGUAGES OBJCXX)
endif()

project(rhi
    VERSION 1.0.0
    DESCRIPTION "render hardware interface"
    LANGUAGES ${_RHI_LANGUAGES}
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    backend/wgpu/*.cpp
    backend/null/*.cpp
    backend/validation/*.cpp
)

if(APPLE)
    file(GLOB_RECURSE APPLE_SOURCES CONFIGURE_DEPENDS
        backend/wgpu/*.mm
    )
    list(APPEND SOURCES ${APPLE_SOURCES})
endif()

set(RHI_DEPS
    wk::core
    # woki::platform
)
set(RHI_PRIVATE_DEPS)

if(EMSCRIPTEN)
    if(NOT DEFINED EMDAWNWEBGPU_PORT OR EMDAWNWEBGPU_PORT STREQUAL "")
        set(_WGPU_PORT emdawnwebgpu)
    else()
        set(_WGPU_PORT "${EMDAWNWEBGPU_PORT}")
    endif()
else()
    find_package(Threads REQUIRED)
    find_package(Dawn CONFIG QUIET)

    if(TARGET dawn::webgpu_dawn)
        set(WGPU_TARGET dawn::webgpu_dawn)
    elseif(TARGET webgpu_dawn)
        set(WGPU_TARGET webgpu_dawn)
    endif()

    if(WGPU_TARGET)
        list(APPEND RHI_DEPS
            "$<BUILD_INTERFACE:${WGPU_TARGET}>"
            "$<INSTALL_INTERFACE:$<1:dawn::webgpu_dawn>>")
    endif()
    list(APPEND RHI_PRIVATE_DEPS
        "$<BUILD_INTERFACE:glfw>"
        "$<INSTALL_INTERFACE:$<1:glfw>>")
endif()

add_module(rhi
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    PUBLIC_DEPENDENCIES ${RHI_DEPS}
    PRIVATE_DEPENDENCIES ${RHI_PRIVATE_DEPS}
)

target_include_directories(wk_rhi
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/backend
)

if(EMSCRIPTEN)
    target_compile_options(wk_rhi PUBLIC
        "--use-port=${_WGPU_PORT}"
    )
endif()

# if(BUILD_TESTING)
#     add_subdirectory(tests)
# endif()
