cmake_minimum_required(VERSION 3.25)

project(rhi
    VERSION 1.0.0
    DESCRIPTION "Render hardware interface"
    LANGUAGES CXX
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)

add_module(rhi
    HEADERS ${HEADERS}
    DEPENDENCIES woki::core
)
