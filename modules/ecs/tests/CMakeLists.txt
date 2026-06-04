cmake_minimum_required(VERSION 3.25)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

add_executable(ecs_tests
    ${SOURCES}
)

target_compile_features(ecs_tests PRIVATE cxx_std_23)
apply_compiler_options(ecs_tests)

target_link_libraries(ecs_tests
    PRIVATE
        woki::ecs
        Catch2::Catch2WithMain
)

add_test(NAME ecs_tests COMMAND ecs_tests)
