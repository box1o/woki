cmake_minimum_required(VERSION 3.25)

function(mark_dependency_system target)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES SYSTEM ON)
    endif()
endfunction()

find_or_add(yaml
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
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

if(NOT EMSCRIPTEN)
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

    find_or_add(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
        DISABLE_INSTALL
        CMAKE_CACHE_VARS
            GLFW_BUILD_WAYLAND=ON
            GLFW_BUILD_X11=ON
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

    find_or_add(Dawn
        GIT_REPOSITORY https://github.com/google/dawn.git
        GIT_TAG main
        FIND_PACKAGE_ARGS CONFIG
        CMAKE_CACHE_VARS
            DAWN_FETCH_DEPENDENCIES=ON
            DAWN_BUILD_TESTS=OFF
            DAWN_ENABLE_VULKAN=ON
            DAWN_ENABLE_D3D11=OFF
            DAWN_ENABLE_D3D12=OFF
            DAWN_ENABLE_METAL=OFF
            DAWN_ENABLE_NULL=ON
            DAWN_ENABLE_DESKTOP_GL=ON
            DAWN_ENABLE_OPENGLES=ON
            DAWN_USE_WAYLAND=ON
            DAWN_USE_X11=ON
            DAWN_USE_GLFW=ON
    )
else()
    message(STATUS "Skipping native graphics dependencies for Emscripten")
endif()
