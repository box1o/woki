set(STUDIO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE STUDIO_SOURCES
    CONFIGURE_DEPENDS
    "${STUDIO_SRC_DIR}/*.cpp"
    "${STUDIO_SRC_DIR}/*.h"
    "${STUDIO_SRC_DIR}/*.hpp"
)

add_executable(studio ${STUDIO_SOURCES})

target_include_directories(studio PRIVATE
    "${STUDIO_SRC_DIR}"
)

target_compile_definitions(studio PRIVATE
    _VERSION="${PROJECT_VERSION}"
)

apply_compiler_options(studio)

target_link_libraries(studio PRIVATE
    wk::core
    wk::math
    wk::rhi
)

if(EMSCRIPTEN)
    target_link_libraries(studio PRIVATE
        emdawnwebgpu::emdawnwebgpu
    )

    target_link_options(studio PRIVATE
        -fexceptions
        -sALLOW_MEMORY_GROWTH=1
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/index.html"
    )

    set_target_properties(studio PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    add_custom_command(
        TARGET studio
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/web/favicon.svg"
            "${CMAKE_BINARY_DIR}/favicon.svg"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/web/status.svg"
            "${CMAKE_BINARY_DIR}/status.svg"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/web/icons"
            "${CMAKE_BINARY_DIR}/icons"
    )
else()
    set_target_properties(studio PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    install(TARGETS studio
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
