include_guard(GLOBAL)

include(FetchContent)

option(WASMTIME_ENABLED "Enable Wasmtime" ON)
option(WASMTIME_USE_SHARED "Use shared Wasmtime library" OFF)

set(WASMTIME_VERSION "45.0.1"
    CACHE STRING
    "Wasmtime release version"
)

set(WASMTIME_PROVIDER "prebuilt"
    CACHE STRING
    "Wasmtime provider: prebuilt, source, or auto"
)

set_property(
    CACHE WASMTIME_PROVIDER
    PROPERTY STRINGS prebuilt source auto
)

set(WASMTIME_ROOT ""
    CACHE PATH
    "Path to an existing Wasmtime installation or source tree"
)


function(_wasmtime_detect_platform os_var arch_var extension_var supported_var)
    set(os "")
    set(arch "")
    set(extension "")
    set(supported FALSE)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(os linux)
        set(extension tar.xz)

        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
            set(arch x86_64)
            set(supported TRUE)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
            set(arch aarch64)
            set(supported TRUE)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^riscv64")
            set(arch riscv64gc)
            set(supported TRUE)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^s390x")
            set(arch s390x)
            set(supported TRUE)
        endif()

    elseif(APPLE)
        set(os macos)
        set(extension tar.xz)

        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
            set(arch aarch64)
            set(supported TRUE)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
            set(arch x86_64)
            set(supported TRUE)
        endif()

    elseif(WIN32)
        set(extension zip)

        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            set(arch x86_64)
            set(supported TRUE)

            if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                set(os mingw)
            else()
                set(os windows)
            endif()
        endif()
    endif()

    set(${os_var} "${os}" PARENT_SCOPE)
    set(${arch_var} "${arch}" PARENT_SCOPE)
    set(${extension_var} "${extension}" PARENT_SCOPE)
    set(${supported_var} "${supported}" PARENT_SCOPE)
endfunction()


function(_wasmtime_register_targets)
    if(TARGET wasmtime AND NOT TARGET wasmtime::wasmtime)
        add_library(wasmtime::wasmtime ALIAS wasmtime)
        set_target_properties(wasmtime PROPERTIES SYSTEM ON)
    endif()

    if(TARGET wasmtime-cpp AND NOT TARGET wasmtime::cpp)
        add_library(wasmtime::cpp ALIAS wasmtime-cpp)
        set_target_properties(wasmtime-cpp PROPERTIES SYSTEM ON)
    endif()
endfunction()


function(_wasmtime_register_cpp include_dir)
    if(
        NOT EXISTS "${include_dir}/wasmtime.hh"
        OR TARGET wasmtime::cpp
        OR NOT TARGET wasmtime::wasmtime
    )
        return()
    endif()

    add_library(wasmtime_cpp INTERFACE)
    add_library(wasmtime::cpp ALIAS wasmtime_cpp)

    target_link_libraries(
        wasmtime_cpp
        INTERFACE
        wasmtime::wasmtime
    )

    target_include_directories(
        wasmtime_cpp
        SYSTEM INTERFACE
        "${include_dir}"
    )

    target_compile_features(
        wasmtime_cpp
        INTERFACE
        cxx_std_17
    )

    if(MSVC)
        target_compile_definitions(
            wasmtime_cpp
            INTERFACE
            WASM_API_EXTERN=
            WASI_API_EXTERN=
        )

        target_link_libraries(
            wasmtime_cpp
            INTERFACE
            ws2_32
            advapi32
            userenv
            ntdll
            shell32
            ole32
            bcrypt
        )
    endif()
endfunction()


function(_wasmtime_find_prebuilt_root result)
    set(root "")

    if(
        WASMTIME_ROOT
        AND EXISTS "${WASMTIME_ROOT}/include/wasmtime.h"
        AND EXISTS "${WASMTIME_ROOT}/include/wasm.h"
    )
        set(root "${WASMTIME_ROOT}")

    elseif(
        EXISTS "${CDEPS_ROOT}/wasmtime-c-api/include/wasmtime.h"
        AND EXISTS "${CDEPS_ROOT}/wasmtime-c-api/include/wasm.h"
    )
        set(root "${CDEPS_ROOT}/wasmtime-c-api")

    else()
        file(GLOB_RECURSE headers
            "${CDEPS_ROOT}/wasmtime-c-api-*/include/wasmtime.h"
        )

        if(headers)
            list(GET headers 0 header)

            get_filename_component(include_dir "${header}" DIRECTORY)
            get_filename_component(root "${include_dir}" DIRECTORY)
        endif()
    endif()

    set(${result} "${root}" PARENT_SCOPE)
endfunction()


function(_wasmtime_find_source_root result)
    set(root "")

    if(
        WASMTIME_ROOT
        AND EXISTS "${WASMTIME_ROOT}/crates/c-api/CMakeLists.txt"
    )
        set(root "${WASMTIME_ROOT}")

    elseif(
        EXISTS "${CDEPS_ROOT}/wasmtime-src/crates/c-api/CMakeLists.txt"
    )
        set(root "${CDEPS_ROOT}/wasmtime-src")
    endif()

    set(${result} "${root}" PARENT_SCOPE)
endfunction()


function(_wasmtime_download_prebuilt)
    _wasmtime_detect_platform(
        os
        arch
        extension
        supported
    )

    if(NOT supported)
        message(
            STATUS
            "Wasmtime: no prebuilt library for "
            "${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}"
        )
        return()
    endif()

    set(tag "v${WASMTIME_VERSION}")
    set(archive "wasmtime-${tag}-${arch}-${os}-c-api.${extension}")

    set(
        url
        "https://github.com/bytecodealliance/wasmtime/releases/download/${tag}/${archive}"
    )

    set(archive_path "${CDEPS_ROOT}/${archive}")
    set(extract_dir "${CDEPS_ROOT}/wasmtime-c-api")
    set(marker "${extract_dir}/.extracted-${tag}")

    file(MAKE_DIRECTORY "${CDEPS_ROOT}")

    if(NOT EXISTS "${archive_path}")
        message(STATUS "Wasmtime: downloading ${archive}")

        file(
            DOWNLOAD
            "${url}"
            "${archive_path}"
            SHOW_PROGRESS
            STATUS download_status
        )

        list(GET download_status 0 download_code)

        if(NOT download_code EQUAL 0)
            list(GET download_status 1 download_message)

            file(REMOVE "${archive_path}")

            message(
                STATUS
                "Wasmtime: download failed (${download_message})"
            )

            return()
        endif()
    else()
        message(STATUS "Wasmtime: using cached ${archive}")
    endif()

    if(NOT EXISTS "${marker}")
        message(STATUS "Wasmtime: extracting ${archive}")

        file(REMOVE_RECURSE "${extract_dir}")
        file(MAKE_DIRECTORY "${extract_dir}")

        file(
            ARCHIVE_EXTRACT
            INPUT "${archive_path}"
            DESTINATION "${extract_dir}"
        )

        file(GLOB_RECURSE headers
            "${extract_dir}/*/include/wasmtime.h"
            "${extract_dir}/include/wasmtime.h"
        )

        if(NOT headers)
            message(
                FATAL_ERROR
                "Wasmtime archive does not contain wasmtime.h"
            )
        endif()

        list(GET headers 0 header)

        get_filename_component(include_dir "${header}" DIRECTORY)
        get_filename_component(prefix "${include_dir}" DIRECTORY)

        if(NOT prefix STREQUAL extract_dir)
            file(
                COPY "${prefix}/include"
                DESTINATION "${extract_dir}"
            )

            if(EXISTS "${prefix}/lib")
                file(
                    COPY "${prefix}/lib"
                    DESTINATION "${extract_dir}"
                )
            endif()
        endif()

        file(WRITE "${marker}" "${tag}")
    endif()
endfunction()


function(_wasmtime_fetch_source)
    set(source_dir "${CDEPS_ROOT}/wasmtime-src")
    set(tag "v${WASMTIME_VERSION}")

    if(EXISTS "${source_dir}/crates/c-api/CMakeLists.txt")
        return()
    endif()

    message(STATUS "Wasmtime: fetching source ${tag}")

    FetchContent_Declare(
        wasmtime_source
        GIT_REPOSITORY https://github.com/bytecodealliance/wasmtime.git
        GIT_TAG ${tag}
        GIT_SHALLOW TRUE
        SOURCE_DIR "${source_dir}"
    )

    FetchContent_GetProperties(wasmtime_source)

    if(NOT wasmtime_source_POPULATED)
        if(POLICY CMP0169)
            cmake_policy(PUSH)
            cmake_policy(SET CMP0169 OLD)
        endif()

        FetchContent_Populate(wasmtime_source)

        if(POLICY CMP0169)
            cmake_policy(POP)
        endif()
    endif()

    if(NOT EXISTS "${source_dir}/crates/c-api/CMakeLists.txt")
        message(
            FATAL_ERROR
            "Wasmtime source does not contain crates/c-api"
        )
    endif()
endfunction()


function(_wasmtime_add_source source_root)
    if(TARGET wasmtime)
        _wasmtime_register_targets()
        return()
    endif()

    find_program(CARGO_EXECUTABLE cargo)

    if(NOT CARGO_EXECUTABLE)
        message(
            STATUS
            "Wasmtime: cargo not found; source build unavailable"
        )
        return()
    endif()

    set(source_dir "${source_root}/crates/c-api")
    set(build_dir "${CMAKE_BINARY_DIR}/wasmtime-c-api")

    if(NOT EXISTS "${source_dir}/CMakeLists.txt")
        return()
    endif()

    get_property(
        build_shared_cached
        CACHE BUILD_SHARED_LIBS
        PROPERTY TYPE
        SET
    )

    if(build_shared_cached)
        get_property(
            previous_build_shared
            CACHE BUILD_SHARED_LIBS
            PROPERTY VALUE
        )
    endif()

    set(
        BUILD_SHARED_LIBS
        ${WASMTIME_USE_SHARED}
        CACHE BOOL
        "Build shared libraries"
        FORCE
    )

    set(
        BUILD_TESTS
        OFF
        CACHE BOOL
        "Build Wasmtime tests"
        FORCE
    )

    set(
        WASMTIME_ALWAYS_BUILD
        ON
        CACHE BOOL
        "Always build Wasmtime"
        FORCE
    )

    message(STATUS "Wasmtime: building from source")

    add_subdirectory(
        "${source_dir}"
        "${build_dir}"
        EXCLUDE_FROM_ALL
    )

    if(build_shared_cached)
        set(
            BUILD_SHARED_LIBS
            "${previous_build_shared}"
            CACHE BOOL
            "Build shared libraries"
            FORCE
        )
    else()
        unset(BUILD_SHARED_LIBS CACHE)
    endif()

    _wasmtime_register_targets()
endfunction()


function(_wasmtime_define_prebuilt root)
    set(include_dir "${root}/include")
    set(lib_dir "${root}/lib")

    if(
        NOT EXISTS "${include_dir}/wasmtime.h"
        OR NOT EXISTS "${include_dir}/wasm.h"
    )
        return()
    endif()

    set(library "")

    if(
        NOT WASMTIME_USE_SHARED
        AND NOT WIN32
        AND EXISTS "${lib_dir}/libwasmtime.a"
    )
        set(library "${lib_dir}/libwasmtime.a")
    else()
        find_library(
            library
            NAMES wasmtime libwasmtime
            PATHS "${lib_dir}"
            NO_DEFAULT_PATH
        )
    endif()

    if(NOT library OR TARGET wasmtime::wasmtime)
        return()
    endif()

    add_library(
        wasmtime::wasmtime
        UNKNOWN
        IMPORTED
        GLOBAL
    )

    set_target_properties(
        wasmtime::wasmtime
        PROPERTIES
        IMPORTED_LOCATION "${library}"
        INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
    )

    if(WASMTIME_USE_SHARED)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set_property(
                TARGET wasmtime::wasmtime
                APPEND PROPERTY
                INTERFACE_LINK_OPTIONS
                "LINKER:-rpath,$ORIGIN"
            )
        endif()

    elseif(WIN32)
        set_property(
            TARGET wasmtime::wasmtime
            APPEND PROPERTY
            INTERFACE_COMPILE_DEFINITIONS
            WASM_API_EXTERN=
            WASI_API_EXTERN=
        )

        set_property(
            TARGET wasmtime::wasmtime
            APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
            ws2_32
            advapi32
            userenv
            ntdll
            shell32
            ole32
            bcrypt
        )

    elseif(APPLE)
        set_property(
            TARGET wasmtime::wasmtime
            APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
            "-framework CoreFoundation"
        )

    else()
        set_property(
            TARGET wasmtime::wasmtime
            APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
            pthread
            dl
            m
        )
    endif()

    message(STATUS "Wasmtime: ${library}")

    _wasmtime_register_cpp("${include_dir}")
endfunction()


function(_wasmtime_build_source)
    _wasmtime_find_source_root(source_root)

    if(NOT source_root)
        _wasmtime_fetch_source()
        _wasmtime_find_source_root(source_root)
    endif()

    if(source_root)
        _wasmtime_add_source("${source_root}")
    endif()
endfunction()


function(setup_wasmtime)
    if(NOT WASMTIME_ENABLED)
        message(STATUS "Wasmtime: disabled")
        return()
    endif()

    if(TARGET wasmtime::wasmtime OR TARGET wasmtime)
        _wasmtime_register_targets()
        return()
    endif()

    string(
        TOLOWER
        "${WASMTIME_PROVIDER}"
        provider
    )

    if(NOT provider MATCHES "^(prebuilt|source|auto)$")
        message(
            FATAL_ERROR
            "WASMTIME_PROVIDER must be prebuilt, source, or auto"
        )
    endif()

    message(STATUS "Wasmtime provider: ${provider}")

    _wasmtime_find_prebuilt_root(root)

    if(root)
        _wasmtime_define_prebuilt("${root}")

        if(TARGET wasmtime::wasmtime)
            return()
        endif()
    endif()

    _wasmtime_find_source_root(source_root)

    if(source_root)
        _wasmtime_add_source("${source_root}")

        if(TARGET wasmtime::wasmtime)
            return()
        endif()
    endif()

    if(provider STREQUAL "prebuilt" OR provider STREQUAL "auto")
        _wasmtime_download_prebuilt()

        _wasmtime_find_prebuilt_root(root)

        if(root)
            _wasmtime_define_prebuilt("${root}")

            if(TARGET wasmtime::wasmtime)
                return()
            endif()
        endif()
    endif()

    if(provider STREQUAL "source" OR provider STREQUAL "auto")
        _wasmtime_build_source()

        if(TARGET wasmtime::wasmtime)
            return()
        endif()
    endif()

    message(
        FATAL_ERROR
        "Wasmtime could not be obtained.\n"
        "  WASMTIME_PROVIDER=${WASMTIME_PROVIDER}\n"
        "  WASMTIME_VERSION=${WASMTIME_VERSION}\n"
        "  WASMTIME_ROOT=${WASMTIME_ROOT}"
    )
endfunction()
