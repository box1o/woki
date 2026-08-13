# set(modules_dir "${CMAKE_SOURCE_DIR}/modules")
# set(module_entries core math config ecs platform asset extension rhi gfx ui)
# foreach(module_entry IN LISTS module_entries)
#     message(STATUS "Module: ${module_entry}")
#     add_subdirectory("${modules_dir}/${module_entry}")
# endforeach()
#
# if(NOT EMSCRIPTEN)
#     add_subdirectory(ext)
# endif()
#
# include(Extensions)
# woki_register_extensions("${CMAKE_SOURCE_DIR}/extensions")
# if(TARGET extension_tests AND TARGET woki_extensions)
#     add_dependencies(extension_tests woki_extensions)
# endif()
#
# add_subdirectory(studio)
#
# install(EXPORT ${namespace}Targets
#     FILE ${namespace}Targets.cmake
#     NAMESPACE ${namespace}::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${namespace})
#
# include(CMakePackageConfigHelpers)
# set(WOKI_PACKAGE_NATIVE TRUE)
# if(EMSCRIPTEN)
#     set(WOKI_PACKAGE_NATIVE FALSE)
# endif()
# set(WOKI_CONFIG_FIND_WASMTIME "")
# if(NOT EMSCRIPTEN AND WOKI_EXTENSION_WITH_WASMTIME)
#     set(WOKI_CONFIG_FIND_WASMTIME "    find_dependency(wasmtime CONFIG)")
# endif()
# configure_package_config_file(
#     "${CMAKE_SOURCE_DIR}/cmake/wokiConfig.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/wokiConfig.cmake"
#     INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${namespace}")
# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/wokiConfigVersion.cmake"
#     VERSION ${PROJECT_VERSION}
#     COMPATIBILITY SameMajorVersion)
# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/wokiConfig.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/wokiConfigVersion.cmake"
#     DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${namespace}")
