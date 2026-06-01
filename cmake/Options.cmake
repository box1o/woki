set(WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat compiler warnings as errors")

function(apply_compiler_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
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
        if(WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
