project(studio)

file(GLOB_RECURSE _HEADERS CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE _SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(
    ${PROJECT_NAME}
    ${_HEADERS}
    ${_SOURCES}
)


find_package(OpenGL REQUIRED)

set(_DEPS
    wk::core
    glfw
    OpenGL::GL
)


target_link_libraries(${PROJECT_NAME} PRIVATE ${_DEPS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        _VERSION="0.0.1"
)
