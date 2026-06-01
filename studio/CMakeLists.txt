set(_EXECUTABLE_NAME studio)

set(_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

file(GLOB_RECURSE _HEADERS
    CONFIGURE_DEPENDS
    ${_SRC_DIR}/*.h
    ${_SRC_DIR}/*.hpp
)

file(GLOB_RECURSE _SOURCES
    CONFIGURE_DEPENDS
    ${_SRC_DIR}/*.cpp
)

add_executable(${_EXECUTABLE_NAME}
    ${_HEADERS}
    ${_SOURCES}
)

target_compile_features(${_EXECUTABLE_NAME} PRIVATE cxx_std_23)
apply_compiler_options(${_EXECUTABLE_NAME})

target_compile_definitions(${_EXECUTABLE_NAME}
    PRIVATE
        _VERSION="${PROJECT_VERSION}"
)

target_include_directories(${_EXECUTABLE_NAME}
    PRIVATE
        ${_SRC_DIR}
)

set(_DEPS
    woki::core
    woki::gui
)

set(_OUTPUT_DIR_NATIVE "${CMAKE_BINARY_DIR}/bin")
set(_OUTPUT_DIR_WEB "${CMAKE_BINARY_DIR}")

if(EMSCRIPTEN)
    message(STATUS "Building studio for web")

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(_WEB_RES_MOUNT "/resources")

    target_compile_definitions(${_EXECUTABLE_NAME}
        PRIVATE
            __WEB
            __WEBGPU_TEST
            WOKI_HAS_DAWN
            WEBGPU_BACKEND_EMSCRIPTEN
    )

    if(NOT DEFINED EMDAWNWEBGPU_PORT OR EMDAWNWEBGPU_PORT STREQUAL "")
        set(_WGPU_PORT "emdawnwebgpu")
        message(STATUS "Using Emscripten WebGPU port: ${_WGPU_PORT}")
    else()
        set(_WGPU_PORT "${EMDAWNWEBGPU_PORT}")
        message(STATUS "Using external emdawnwebgpu port: ${_WGPU_PORT}")
    endif()

    target_compile_options(${_EXECUTABLE_NAME} PRIVATE
        "--use-port=${_WGPU_PORT}"
    )

    if(EXISTS "${_RES_DIR}")
        target_link_options(${_EXECUTABLE_NAME} PRIVATE
            "--preload-file" "${_RES_DIR}@${_WEB_RES_MOUNT}"
        )
        target_compile_definitions(${_EXECUTABLE_NAME} PRIVATE
            __RES_ROOT="${_WEB_RES_MOUNT}"
        )
    endif()

    target_link_options(${_EXECUTABLE_NAME} PRIVATE
        "--use-port=${_WGPU_PORT}"
        "-sUSE_GLFW=3"
        "-sASYNCIFY"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sASSERTIONS=1"
        "--shell-file" "${CMAKE_SOURCE_DIR}/web/index.html"
    )

    set_target_properties(${_EXECUTABLE_NAME} PROPERTIES
        OUTPUT_NAME "studio"
        RUNTIME_OUTPUT_DIRECTORY "${_OUTPUT_DIR_WEB}"
    )

    add_custom_command(
        TARGET ${_EXECUTABLE_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/web/favicon.svg"
            "${CMAKE_BINARY_DIR}/favicon.svg"
    )
else()
    message(STATUS "Building studio for native")

    list(APPEND _DEPS glfw)

    find_package(Threads REQUIRED)

    list(APPEND _DEPS Threads::Threads)
    list(APPEND _DEPS msdfgen::msdfgen)

    if(NOT TARGET dawn::webgpu_dawn)
        find_package(Dawn CONFIG QUIET)
    endif()

    if(TARGET dawn::webgpu_dawn)
        message(STATUS "Dawn found - enabling WebGPU renderer")
        list(APPEND _DEPS dawn::webgpu_dawn)
        target_compile_definitions(${_EXECUTABLE_NAME} PRIVATE WOKI_HAS_DAWN)
    elseif(TARGET webgpu_dawn)
        message(STATUS "Dawn found (legacy target) - enabling WebGPU renderer")
        list(APPEND _DEPS webgpu_dawn)
        target_compile_definitions(${_EXECUTABLE_NAME} PRIVATE WOKI_HAS_DAWN)
    else()
        message(STATUS "Dawn not found - building studio without WebGPU renderer")
    endif()

    set_target_properties(${_EXECUTABLE_NAME} PROPERTIES
        OUTPUT_NAME "studio"
        RUNTIME_OUTPUT_DIRECTORY "${_OUTPUT_DIR_NATIVE}"
    )

    if(APPLE)
        set_target_properties(${_EXECUTABLE_NAME} PROPERTIES MACOSX_BUNDLE ON)
    endif()
endif()

target_link_libraries(${_EXECUTABLE_NAME} PRIVATE ${_DEPS})

install(TARGETS ${_EXECUTABLE_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(EXISTS "${_RES_DIR}" AND NOT EMSCRIPTEN)
    install(
        DIRECTORY ${_RES_DIR}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/woki
    )

    add_custom_command(
        TARGET ${_EXECUTABLE_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_RES_DIR}"
            "${_OUTPUT_DIR_NATIVE}/resources"
    )
endif()
