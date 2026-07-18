cmake_minimum_required(VERSION 3.25)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

add_module_test(gfx_tests
    SOURCES
        ${SOURCES}
    LIBRARIES
        woki::gfx
        Catch2::Catch2WithMain
)

target_include_directories(gfx_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
