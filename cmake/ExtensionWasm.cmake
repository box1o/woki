# Shared wasm guest extension build + LSP (compile_commands) support.
#
# Usage from extensions/<name>/CMakeLists.txt:
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ExtensionWasm.cmake)
#   woki_add_extension_wasm(src/plugin.cpp)

function(woki_add_extension_wasm source_file)
    if(NOT WOKI_SDK_DIR)
        get_filename_component(_woki_root "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
        set(WOKI_SDK_DIR "${_woki_root}/modules/extension/sdk" CACHE PATH "Woki extension SDK directory")
    endif()

    set(_source "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")

    if(_source MATCHES "\\.c$")
        set(_std_flag -std=c17)
        if(NOT WOKI_WASM_COMPILER)
            find_program(WOKI_WASM_COMPILER clang REQUIRED)
        endif()
    else()
        set(_std_flag -std=c++23)
        if(NOT WOKI_WASM_COMPILER)
            find_program(WOKI_WASM_COMPILER clang++ REQUIRED)
        endif()
    endif()

    set(_wasm "${CMAKE_CURRENT_SOURCE_DIR}/extension.wasm")

    set(_guest_flags
        --target=wasm32-unknown-unknown
        -nostdlib
        -I${WOKI_SDK_DIR}
        ${_std_flag}
    )

    # Real compile target so CMAKE_EXPORT_COMPILE_COMMANDS populates clangd entries.
    add_library(extension_guest OBJECT EXCLUDE_FROM_ALL "${_source}")
    target_compile_options(extension_guest PRIVATE ${_guest_flags})
    target_include_directories(extension_guest PRIVATE "${WOKI_SDK_DIR}")
    if(_source MATCHES "\\.c$")
        set_target_properties(extension_guest PROPERTIES LINKER_LANGUAGE C)
    else()
        set_target_properties(extension_guest PROPERTIES LINKER_LANGUAGE CXX)
    endif()

    add_custom_command(
        OUTPUT "${_wasm}"
        COMMAND ${WOKI_WASM_COMPILER}
            --target=wasm32-unknown-unknown
            ${_std_flag}
            -O2
            -nostdlib
            -I "${WOKI_SDK_DIR}"
            -Wl,--no-entry
            -Wl,--allow-undefined
            -Wl,--export-memory
            -Wl,--export=ext_api_version
            -Wl,--export=ext_init
            -Wl,--export=ext_on_tick
            -Wl,--export=ext_on_event
            -Wl,--export=ext_on_unload
            -Wl,--export=ext_alloc
            -Wl,--export=ext_free
            -o "${_wasm}"
            "${_source}"
        DEPENDS "${_source}" extension_guest
        VERBATIM
    )

    add_custom_target(extension ALL DEPENDS "${_wasm}")
endfunction()
