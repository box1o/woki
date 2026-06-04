cmake_minimum_required(VERSION 3.25)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

add_executable(core_tests
    ${SOURCES}
)

target_compile_features(core_tests PRIVATE cxx_std_23)
apply_compiler_options(core_tests)

target_link_libraries(core_tests
    PRIVATE
        woki::core
        Catch2::Catch2WithMain
)

add_test(NAME core_tests COMMAND core_tests)
