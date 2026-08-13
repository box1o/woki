function(mark_dependency_system target)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES SYSTEM ON)
    endif()
endfunction()

find_or_add(yaml
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
    CMAKE_CACHE_VARS
        "CMAKE_POLICY_VERSION_MINIMUM=3.5"
)
mark_dependency_system(yaml-cpp)

find_or_add(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    CMAKE_CACHE_VARS
        "CXXOPTS_STANDALONE_PROJECT=OFF"
)
mark_dependency_system(cxxopts)

find_or_add(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
mark_dependency_system(Catch2)
mark_dependency_system(Catch2WithMain)

if(EMSCRIPTEN)
    add_library(emdawnwebgpu_port INTERFACE)
    add_library(emdawnwebgpu::emdawnwebgpu ALIAS emdawnwebgpu_port)

    target_compile_options(emdawnwebgpu_port INTERFACE
        "--use-port=emdawnwebgpu"
    )

    target_link_options(emdawnwebgpu_port INTERFACE
        "--use-port=emdawnwebgpu"
    )

    message(STATUS "Using Emdawnwebgpu for web")
    return()
endif()

set(build_shared_libs "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)

find_or_add(fastgltf
    NO_SYSTEM
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG v0.9.0
    DISABLE_INSTALL
    CMAKE_CACHE_VARS
        "FASTGLTF_ENABLE_TESTS=OFF"
        "FASTGLTF_ENABLE_EXAMPLES=OFF"
        "FASTGLTF_ENABLE_DOCS=OFF"
)
mark_dependency_system(fastgltf)

find_or_add(meshoptimizer
    NO_SYSTEM
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG v0.25
    DISABLE_INSTALL
    CMAKE_CACHE_VARS
        "MESHOPT_BUILD_DEMO=OFF"
        "MESHOPT_BUILD_GLTFPACK=OFF"
)
mark_dependency_system(meshoptimizer)

set(UNZIP_FOUND OFF CACHE BOOL "Use Assimp's pinned minizip sources" FORCE)

find_or_add(assimp
    NO_SYSTEM
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.2
    DISABLE_INSTALL
    CMAKE_CACHE_VARS
        "ASSIMP_BUILD_TESTS=OFF"
        "ASSIMP_BUILD_ASSIMP_TOOLS=OFF"
        "ASSIMP_BUILD_SAMPLES=OFF"
        "ASSIMP_BUILD_ZLIB=ON"
        "ASSIMP_INSTALL=OFF"
        "ASSIMP_WARNINGS_AS_ERRORS=OFF"
        "ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=OFF"
        "ASSIMP_BUILD_MINIZIP=ON"
)

target_include_directories(
    assimp
    PRIVATE
    "${CDEPS_ROOT}/assimp-src/contrib/unzip"
)

mark_dependency_system(assimp)

find_or_add(KTX
    NO_SYSTEM
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG 936b655d10fe75f900967f524ba31005bebcbb47
    RECURSE_SUBMODULES
    DISABLE_INSTALL
    CMAKE_CACHE_VARS
        "KTX_FEATURE_DOC=OFF"
        "KTX_FEATURE_GL_UPLOAD=OFF"
        "KTX_FEATURE_JNI=OFF"
        "KTX_FEATURE_KTX1=ON"
        "KTX_FEATURE_LOADTEST_APPS=OFF"
        "KTX_FEATURE_TESTS=OFF"
        "KTX_FEATURE_TOOLS=OFF"
        "KTX_FEATURE_VK_UPLOAD=OFF"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
   AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    foreach(target ktx ktx_read)
        if(TARGET ${target})
            get_target_property(options ${target} COMPILE_OPTIONS)

            if(options)
                list(FILTER options EXCLUDE REGEX "ffp-model=precise")
                set_target_properties(
                    ${target}
                    PROPERTIES COMPILE_OPTIONS "${options}"
                )
            endif()
        endif()
    endforeach()
endif()

mark_dependency_system(ktx)
mark_dependency_system(ktx_read)

include(FetchContent)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(stb)

add_library(wk_stb INTERFACE)

target_include_directories(
    wk_stb
    SYSTEM INTERFACE
    "${stb_SOURCE_DIR}"
)

find_package(LibArchive QUIET)

if(NOT LibArchive_FOUND)
    find_or_add(LibArchive
        NO_SYSTEM
        GIT_REPOSITORY https://github.com/libarchive/libarchive.git
        GIT_TAG v3.7.7
        DISABLE_INSTALL
        CMAKE_CACHE_VARS
            "ENABLE_TEST=OFF"
            "ENABLE_WERROR=OFF"
            "ENABLE_INSTALL=OFF"
            "ENABLE_TAR=OFF"
            "ENABLE_CPIO=OFF"
            "ENABLE_CAT=OFF"
    )
endif()

if(TARGET archive)
    mark_dependency_system(archive)
elseif(TARGET archive_static)
    mark_dependency_system(archive_static)
endif()

add_library(wk_libarchive INTERFACE)
add_library(wk::libarchive ALIAS wk_libarchive)


if(TARGET LibArchive::LibArchive)
    target_link_libraries(wk_libarchive INTERFACE LibArchive::LibArchive)
elseif(TARGET archive)
    target_link_libraries(wk_libarchive INTERFACE archive)
elseif(TARGET archive_static)
    target_link_libraries(wk_libarchive INTERFACE archive_static)
else()
    message(FATAL_ERROR "LibArchive target not available")
endif()

include(Wasmtime)
setup_wasmtime()

find_or_add(fmt
    NO_SYSTEM
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)

target_compile_definitions(fmt PUBLIC FMT_CONSTEVAL=)

find_or_add(spdlog
    NO_SYSTEM
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.1
    CMAKE_CACHE_VARS
        "SPDLOG_FMT_EXTERNAL=ON"
)

set(glfw_wayland OFF)
set(glfw_x11 OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(glfw_wayland ON)
    set(glfw_x11 ON)
endif()

find_or_add(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    DISABLE_INSTALL
    CMAKE_CACHE_VARS
        "GLFW_BUILD_WAYLAND=${glfw_wayland}"
        "GLFW_BUILD_X11=${glfw_x11}"
)

find_or_add(msdfgen
    NO_SYSTEM
    GIT_REPOSITORY https://github.com/Chlumsky/msdfgen.git
    GIT_TAG v1.13
    CMAKE_CACHE_VARS
        "MSDFGEN_BUILD_STANDALONE=OFF"
        "MSDFGEN_USE_VCPKG=OFF"
        "MSDFGEN_USE_SKIA=OFF"
        "MSDFGEN_DISABLE_SVG=ON"
        "MSDFGEN_DISABLE_PNG=ON"
        "MSDFGEN_INSTALL=OFF"
)

set(dawn_vulkan OFF)
set(dawn_d3d12 OFF)
set(dawn_metal OFF)
set(dawn_desktop_gl OFF)
set(dawn_opengles OFF)
set(dawn_wayland OFF)
set(dawn_x11 OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(dawn_vulkan ON)
    set(dawn_desktop_gl ON)
    set(dawn_opengles ON)
    set(dawn_wayland ON)
    set(dawn_x11 ON)
elseif(APPLE)
    set(dawn_metal ON)
elseif(WIN32)
    set(dawn_d3d12 ON)
endif()

find_or_add(Dawn
    GIT_REPOSITORY https://github.com/google/dawn.git
    GIT_TAG 9fd4a731474f66c1ce5465f9b13919ad7487315d
    FIND_PACKAGE_ARGS CONFIG
    CMAKE_CACHE_VARS
        "DAWN_FETCH_DEPENDENCIES=ON"
        "DAWN_BUILD_TESTS=OFF"
        "DAWN_ENABLE_VULKAN=${dawn_vulkan}"
        "DAWN_ENABLE_D3D11=OFF"
        "DAWN_ENABLE_D3D12=${dawn_d3d12}"
        "DAWN_ENABLE_METAL=${dawn_metal}"
        "DAWN_ENABLE_NULL=ON"
        "DAWN_ENABLE_DESKTOP_GL=${dawn_desktop_gl}"
        "DAWN_ENABLE_OPENGLES=${dawn_opengles}"
        "DAWN_USE_WAYLAND=${dawn_wayland}"
        "DAWN_USE_X11=${dawn_x11}"
        "DAWN_USE_GLFW=ON"
)

set(BUILD_SHARED_LIBS "${build_shared_libs}")
