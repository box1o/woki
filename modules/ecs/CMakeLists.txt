cmake_minimum_required(VERSION 3.25)

project(ecs
    VERSION 1.0.0
    DESCRIPTION "Entity component system module"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

set(ECS_DEPS
    woki::core
)

add_module(ecs
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    DEPENDENCIES ${ECS_DEPS}
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
