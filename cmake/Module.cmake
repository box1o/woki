function(add_module name)
    set(multi_value_args SOURCES HEADERS DEPENDENCIES)
    cmake_parse_arguments(ARG "" "" "${multi_value_args}" ${ARGN})

    if(NOT ARG_HEADERS)
        message(FATAL_ERROR "Module '${name}' must define HEADERS")
    endif()

    set(target "${namespace}_${name}")

    add_library(${target}
        ${ARG_SOURCES}
        ${ARG_HEADERS}
    )

    add_library(${namespace}::${name} ALIAS ${target})

    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    if(ARG_DEPENDENCIES)
        target_link_libraries(${target} PUBLIC ${ARG_DEPENDENCIES})
    endif()

    target_compile_features(${target} PUBLIC cxx_std_23)

    apply_compiler_options(${target})

    install(TARGETS ${target}
        EXPORT ${namespace}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()
