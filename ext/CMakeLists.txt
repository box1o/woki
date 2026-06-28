cmake_minimum_required(VERSION 3.25)

project(wokiext
    VERSION 0.1.0
    DESCRIPTION "Woki extension development command line tool"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(wokiext
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(wokiext
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(wokiext PRIVATE cxx_std_23)
apply_compiler_options(wokiext)

target_link_libraries(wokiext
    PRIVATE
        woki::extension
        cxxopts::cxxopts
)

if(NOT EMSCRIPTEN)
    target_link_libraries(wokiext PRIVATE woki::libarchive)
endif()

target_compile_definitions(wokiext PRIVATE
    WOKI_MANIFEST_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/modules/extension/schema/manifest.schema.json"
)

install(TARGETS wokiext
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
