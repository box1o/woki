cmake_minimum_required(VERSION 3.25)

set(WOKI_WASMTIME_ROOT "" CACHE PATH "Wasmtime C API install root (overrides auto-download)")

set(WOKI_WASMTIME_PROVIDER "prebuilt" CACHE STRING
    "How to obtain Wasmtime C API: prebuilt (download release), source (cargo build), auto (prebuilt then source)")
set_property(CACHE WOKI_WASMTIME_PROVIDER PROPERTY STRINGS prebuilt source auto)

option(WOKI_WASMTIME_USE_SHARED "Link against shared libwasmtime instead of static" OFF)

# Deprecated: use WOKI_WASMTIME_PROVIDER=source instead.
option(WOKI_WASMTIME_BUILD_FROM_SOURCE
    "Deprecated; equivalent to WOKI_WASMTIME_PROVIDER=source"
    OFF
)
if(WOKI_WASMTIME_BUILD_FROM_SOURCE)
    set(WOKI_WASMTIME_PROVIDER "source" CACHE STRING "" FORCE)
endif()

include("${CMAKE_SOURCE_DIR}/cmake/Wasmtime.cmake")
woki_setup_wasmtime()
