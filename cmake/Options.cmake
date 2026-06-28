set(WARNINGS_AS_ERRORS ON CACHE BOOL "Treat compiler warnings as errors")

if(NOT EMSCRIPTEN)
    option(WOKI_EXTENSION_WITH_WASMTIME
        "Enable Wasmtime wasm runtime in woki::extension (downloads or builds Wasmtime C API)"
        ON
    )
endif()

function(apply_compiler_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /wd4201
        )
        if(WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wno-pedantic
            -Wno-unused-parameter
            -Wno-deprecated-declarations
        )
        if(EMSCRIPTEN)
            target_compile_options(${target} PRIVATE -fexceptions)
        endif()
        if(WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
