# Register out-of-tree extension guest builds (isolated from the host toolchain).
#
# Each extensions/<name>/ project is configured under
# ${CMAKE_BINARY_DIR}/extensions/<name> so guest wasm flags never leak into studio.

function(woki_register_extensions extensions_dir)
    if(NOT EXISTS "${extensions_dir}")
        return()
    endif()

    if(NOT TARGET woki_extensions)
        add_custom_target(woki_extensions ALL)
    endif()

    file(GLOB extension_entries RELATIVE "${extensions_dir}" "${extensions_dir}/*")
    foreach(extension_entry IN LISTS extension_entries)
        set(extension_dir "${extensions_dir}/${extension_entry}")
        if(NOT IS_DIRECTORY "${extension_dir}")
            continue()
        endif()
        if(NOT EXISTS "${extension_dir}/CMakeLists.txt")
            continue()
        endif()

        set(extension_build_dir "${CMAKE_BINARY_DIR}/extensions/${extension_entry}")
        string(MAKE_C_IDENTIFIER "woki_extension_${extension_entry}" extension_target)

        add_custom_target(${extension_target}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${extension_build_dir}"
            COMMAND ${CMAKE_COMMAND}
                -B "${extension_build_dir}"
                -S "${extension_dir}"
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                -DWOKI_REPO_ROOT=${CMAKE_SOURCE_DIR}
            COMMAND ${CMAKE_COMMAND} --build "${extension_build_dir}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Building extension ${extension_entry}"
            VERBATIM
        )
        add_dependencies(woki_extensions ${extension_target})
    endforeach()
endfunction()
