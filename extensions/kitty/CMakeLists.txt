cmake_minimum_required(VERSION 3.25)
find_program(WOKI_WASM_COMPILER clang++ REQUIRED)
set(CMAKE_CXX_COMPILER "${WOKI_WASM_COMPILER}" CACHE STRING "Wasm extension compiler" FORCE)
project(woki_extension LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(WOKI_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
if(EXISTS "${WOKI_REPO_ROOT}/cmake/ExtensionWasm.cmake")
    include(${WOKI_REPO_ROOT}/cmake/ExtensionWasm.cmake)
    woki_add_extension_wasm(src/plugin.cpp)
else()
    message(FATAL_ERROR
        "woki_add_extension_wasm requires cmake/ExtensionWasm.cmake. "
        "Create extensions inside the woki repository under extensions/.")
endif()
