cmake_minimum_required(VERSION 3.25)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

add_module_test(ecs_tests
    SOURCES
        ${SOURCES}
    LIBRARIES
        woki::ecs
        Catch2::Catch2WithMain
)
