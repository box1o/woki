cmake_minimum_required(VERSION 3.25)

project(extension
    VERSION 1.0.0
    DESCRIPTION "extension host runtime"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)

file(GLOB_RECURSE SDK_HEADERS CONFIGURE_DEPENDS
    sdk/*.h
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

list(FILTER SOURCES EXCLUDE REGEX ".*/src/wasm/wasmtime_engine\\.cpp$")

set(EXTENSION_DEPS
    woki::core
    yaml-cpp::yaml-cpp
)

if(NOT EMSCRIPTEN)
    if(NOT TARGET woki::libarchive)
        message(FATAL_ERROR
            "woki::extension requires woki::libarchive on native builds. "
            "Install libarchive development files or allow FetchContent in 3rdparty/CMakeLists.txt.")
    endif()
    list(APPEND EXTENSION_DEPS woki::libarchive)

    if(WOKI_EXTENSION_WITH_WASMTIME)
        if(NOT TARGET wasmtime::cpp AND NOT TARGET wasmtime::wasmtime)
            message(FATAL_ERROR
                "WOKI_EXTENSION_WITH_WASMTIME=ON but Wasmtime is unavailable. "
                "Re-run cmake configure to fetch or build the Wasmtime C API.")
        endif()
        if(TARGET wasmtime::cpp)
            list(APPEND EXTENSION_DEPS wasmtime::cpp)
        else()
            list(APPEND EXTENSION_DEPS wasmtime::wasmtime)
        endif()
    endif()
endif()

set(EXTENSION_SOURCES ${SOURCES})
if(NOT EMSCRIPTEN AND WOKI_EXTENSION_WITH_WASMTIME)
    list(APPEND EXTENSION_SOURCES src/wasm/wasmtime_engine.cpp)
endif()

add_module(extension
    SOURCES ${EXTENSION_SOURCES}
    HEADERS ${HEADERS} ${SDK_HEADERS}
    DEPENDENCIES ${EXTENSION_DEPS}
)

target_include_directories(woki_extension
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk>
)

if(NOT EMSCRIPTEN)
    if(TARGET woki_thirdparty_deps)
        add_dependencies(woki_extension woki_thirdparty_deps)
    endif()
endif()

if(NOT EMSCRIPTEN AND WOKI_EXTENSION_WITH_WASMTIME)
    target_compile_definitions(woki_extension PUBLIC WOKI_EXTENSION_WITH_WASMTIME=1)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sdk/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/woki
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schema/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/woki/extension/schema
    FILES_MATCHING PATTERN "*.json"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/wit/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/woki/extension/wit
    FILES_MATCHING PATTERN "*.wit"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/web/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/woki/extension/web
    FILES_MATCHING PATTERN "*.js"
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
