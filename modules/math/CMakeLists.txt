project(math
    VERSION 1.0.0
    DESCRIPTION "header-only math lib"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)

add_module(${PROJECT_NAME}
    HEADER_ONLY
    HEADERS ${HEADERS}
)

install(
    DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wk/docs/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.md"
)
