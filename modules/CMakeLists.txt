# ================================================================================
# Iterator over modules
# ================================================================================

set(entries core math platform)

foreach(entry IN LISTS entries)
    message(STATUS "Module: ${entry}")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${entry}")
endforeach()
