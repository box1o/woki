set(STUDIO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE STUDIO_SOURCES
    CONFIGURE_DEPENDS
    "${STUDIO_SRC_DIR}/*.cpp"
    "${STUDIO_SRC_DIR}/*.h"
    "${STUDIO_SRC_DIR}/*.hpp"
)

add_executable(studio ${STUDIO_SOURCES})

target_include_directories(studio PRIVATE "${STUDIO_SRC_DIR}")

target_compile_definitions(studio PRIVATE
    _VERSION="${PROJECT_VERSION}"
)

apply_compiler_options(studio)

set_target_properties(studio PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(studio PRIVATE
    wk::math
#     wk::core
#     wk::platform
#     wk::rhi
#     wk::gfx
#     wk::ui
)

install(TARGETS studio
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)
