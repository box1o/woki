function(add_module name)
    set(options NO_INSTALL_HEADERS HEADER_ONLY)
    set(multi_value_args SOURCES HEADERS DEPENDENCIES PUBLIC_DEPENDENCIES PRIVATE_DEPENDENCIES)
    cmake_parse_arguments(ARG "${options}" "" "${multi_value_args}" ${ARGN})

    if(NOT ARG_HEADERS)
        message(FATAL_ERROR "Module '${name}' must define HEADERS")
    endif()

    set(target "${namespace}_${name}")

    if(ARG_HEADER_ONLY)
        if(ARG_SOURCES)
            message(FATAL_ERROR "Header-only module '${name}' cannot define SOURCES")
        endif()
        add_library(${target} INTERFACE ${ARG_HEADERS})
        set(module_scope INTERFACE)
    else()
        add_library(${target}
            ${ARG_SOURCES}
            ${ARG_HEADERS}
        )
        set(module_scope PUBLIC)
    endif()

    add_library(${namespace}::${name} ALIAS ${target})
    set_target_properties(${target} PROPERTIES EXPORT_NAME ${name})

    target_include_directories(${target}
        ${module_scope}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    if(NOT ARG_HEADER_ONLY)
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    endif()

    if(ARG_DEPENDENCIES OR ARG_PUBLIC_DEPENDENCIES)
        target_link_libraries(${target} ${module_scope} ${ARG_DEPENDENCIES} ${ARG_PUBLIC_DEPENDENCIES})
    endif()

    if(ARG_PRIVATE_DEPENDENCIES)
        if(ARG_HEADER_ONLY)
            message(FATAL_ERROR "Header-only module '${name}' cannot define PRIVATE_DEPENDENCIES")
        endif()
        target_link_libraries(${target} PRIVATE ${ARG_PRIVATE_DEPENDENCIES})
    endif()

    target_compile_features(${target} ${module_scope} cxx_std_23)

    #FIXME: implment this function
    # if(NOT ARG_HEADER_ONLY)
    #     apply_compiler_options(${target})
    # endif()

    install(TARGETS ${target}
        EXPORT ${namespace}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    if(NOT ARG_NO_INSTALL_HEADERS)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()
endfunction()
