project(studio)

file(GLOB_RECURSE _HEADERS CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE _SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(
    ${PROJECT_NAME}
    ${_HEADERS}
    ${_SOURCES}
)

set(_DEPS
    wk::core
    wk::math
    wk::platform
)

if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    target_compile_options(${PROJECT_NAME} PRIVATE "--use-port=contrib.glfw3")

    target_link_options(${PROJECT_NAME} PRIVATE
        "--use-port=contrib.glfw3"
        "--emrun"
        "--shell-file=${CMAKE_SOURCE_DIR}/web/index.html"
    )

    file(COPY
        ${CMAKE_SOURCE_DIR}/web/favicon.svg
        ${CMAKE_SOURCE_DIR}/web/status.svg
        ${CMAKE_SOURCE_DIR}/web/woki-sad.svg
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    find_package(OpenGL REQUIRED)
    list(APPEND _DEPS glfw OpenGL::GL)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${_DEPS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        _VERSION="0.0.1"
)
