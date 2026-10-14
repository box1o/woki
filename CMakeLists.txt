cmake_minimum_required(VERSION 4.4)
project(woki)

# ================================================================================
# CMake configuration for the project
# ================================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
set(namespace "wk" CACHE STRING "namespace (this is used for libs alias)")

# ================================================================================
# CMake hellper functions
# ================================================================================
include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(deps)
include(module)




add_subdirectory(3rdparty)
add_subdirectory(modules)
add_subdirectory(studio)

