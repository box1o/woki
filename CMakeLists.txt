cmake_minimum_required(VERSION 3.25)

set(WOKI_PROJECT_LANGUAGES C CXX)
if(APPLE)
    list(APPEND WOKI_PROJECT_LANGUAGES OBJCXX)
endif()

project(woki VERSION 0.0.1 LANGUAGES ${WOKI_PROJECT_LANGUAGES})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(namespace "woki" CACHE STRING "Project namespace")

include(Options)
include(Dependencies)
include(Module)
include(CTest)

if(BUILD_TESTING)
    add_custom_target(woki_tests)
endif()

message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:       ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ standard:   C++${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

add_subdirectory(3rdparty)

set(modules_dir "${CMAKE_SOURCE_DIR}/modules")
file(GLOB module_entries RELATIVE "${modules_dir}" "${modules_dir}/*")

foreach(module_entry IN LISTS module_entries)
    if(IS_DIRECTORY "${modules_dir}/${module_entry}")
        message(STATUS "Module: ${module_entry}")
        add_subdirectory("${modules_dir}/${module_entry}")
    endif()
endforeach()

add_subdirectory(ext)

if(NOT EMSCRIPTEN)
    set(extensions_dir "${CMAKE_SOURCE_DIR}/extensions")
    if(EXISTS "${extensions_dir}")
        file(GLOB extension_entries RELATIVE "${extensions_dir}" "${extensions_dir}/*")
        add_custom_target(woki_extensions ALL)

        foreach(extension_entry IN LISTS extension_entries)
            set(extension_dir "${extensions_dir}/${extension_entry}")
            if(IS_DIRECTORY "${extension_dir}" AND EXISTS "${extension_dir}/CMakeLists.txt")
                set(extension_build_dir "${CMAKE_BINARY_DIR}/extensions/${extension_entry}")
                string(REGEX REPLACE "[^A-Za-z0-9_]" "_" extension_target_name
                    "woki_extension_${extension_entry}")
                add_custom_target(${extension_target_name}
                    COMMAND ${CMAKE_COMMAND}
                        -B "${extension_build_dir}"
                        -S "${extension_dir}"
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                    COMMAND ${CMAKE_COMMAND}
                        --build "${extension_build_dir}"
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    COMMENT "Building extension ${extension_entry}"
                    VERBATIM
                )
                add_dependencies(woki_extensions ${extension_target_name})
            endif()
        endforeach()
    endif()
endif()

add_subdirectory(studio)
