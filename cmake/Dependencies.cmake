if(NOT DEFINED ROOT_DIR)
    set(ROOT_DIR "${CMAKE_SOURCE_DIR}")
endif()

include(FetchContent)

set(CDEPS_ROOT "${ROOT_DIR}/3rdparty/.deps" CACHE PATH "")
set(FETCHCONTENT_BASE_DIR "${CDEPS_ROOT}" CACHE PATH "")

message(STATUS "Deps directory: ${CDEPS_ROOT}")
message(STATUS "FetchContent base directory: ${FETCHCONTENT_BASE_DIR}")

function(find_or_add package_name)
    set(options REQUIRED RECURSE_SUBMODULES NO_SHALLOW DISABLE_INSTALL NO_SYSTEM)
    set(one_value_args GIT_REPOSITORY GIT_TAG POST_FETCH_SCRIPT)
    set(multi_value_args FIND_PACKAGE_ARGS POST_FETCH_COMMANDS CMAKE_CACHE_VARS)
    cmake_parse_arguments(
        ARG
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
    )

    if(NOT ARG_NO_SYSTEM)
        find_package(${package_name} ${ARG_FIND_PACKAGE_ARGS} QUIET)
        if(${package_name}_FOUND)
            message(STATUS "Found ${package_name} (system)")
            return()
        endif()
    endif()

    if(NOT ARG_GIT_REPOSITORY)
        if(ARG_REQUIRED)
            message(FATAL_ERROR
                "${package_name} not found and no GIT_REPOSITORY provided")
        endif()
        message(WARNING "${package_name} not found")
        return()
    endif()

    string(TOLOWER "${package_name}" package_name_lower)

    set(PKG_SOURCE_DIR "${CDEPS_ROOT}/${package_name_lower}-src")
    set(PKG_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${package_name_lower}-build")

    set(ALREADY_DOWNLOADED FALSE)
    if(EXISTS "${PKG_SOURCE_DIR}/CMakeLists.txt")
        message(STATUS "Using cached ${package_name} from ${PKG_SOURCE_DIR}")
        set(ALREADY_DOWNLOADED TRUE)
    else()
        message(STATUS "Fetching ${package_name} from ${ARG_GIT_REPOSITORY}")
    endif()

    if(ARG_CMAKE_CACHE_VARS)
        foreach(cache_var ${ARG_CMAKE_CACHE_VARS})
            if(cache_var MATCHES "^([^=]+)=(.*)$")
                set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2} CACHE INTERNAL "")
            endif()
        endforeach()
    endif()

    if(ALREADY_DOWNLOADED)
        set(MARKER_FILE "${PKG_SOURCE_DIR}/.postfetch_done")

        if(ARG_POST_FETCH_SCRIPT OR ARG_POST_FETCH_COMMANDS)
            if(NOT EXISTS "${MARKER_FILE}")
                message(STATUS "Running post-fetch operations for ${package_name}")

                if(ARG_POST_FETCH_SCRIPT)
                    set(SCRIPT_PATH "${PKG_SOURCE_DIR}/${ARG_POST_FETCH_SCRIPT}")
                    if(EXISTS "${SCRIPT_PATH}")
                        message(STATUS "Running post-fetch script: ${ARG_POST_FETCH_SCRIPT}")
                        execute_process(
                            COMMAND chmod +x ${SCRIPT_PATH}
                            WORKING_DIRECTORY ${PKG_SOURCE_DIR}
                        )
                        execute_process(
                            COMMAND ${SCRIPT_PATH}
                            WORKING_DIRECTORY ${PKG_SOURCE_DIR}
                            RESULT_VARIABLE SCRIPT_RESULT
                        )
                        if(NOT SCRIPT_RESULT EQUAL 0)
                            message(FATAL_ERROR "Post-fetch script failed: ${ARG_POST_FETCH_SCRIPT}")
                        endif()
                    else()
                        message(WARNING "Post-fetch script not found: ${SCRIPT_PATH}")
                    endif()
                endif()

                if(ARG_POST_FETCH_COMMANDS)
                    foreach(cmd ${ARG_POST_FETCH_COMMANDS})
                        message(STATUS "Running post-fetch command: ${cmd}")
                        separate_arguments(cmd_list UNIX_COMMAND "${cmd}")
                        execute_process(
                            COMMAND ${cmd_list}
                            WORKING_DIRECTORY ${PKG_SOURCE_DIR}
                            RESULT_VARIABLE CMD_RESULT
                        )
                        if(NOT CMD_RESULT EQUAL 0)
                            message(FATAL_ERROR "Post-fetch command failed: ${cmd}")
                        endif()
                    endforeach()
                endif()

                file(WRITE "${MARKER_FILE}" "Post-fetch operations completed")
            endif()
        endif()

        if(ARG_DISABLE_INSTALL)
            set(CMAKE_SKIP_INSTALL_RULES TRUE)
            set(SKIP_INSTALL_ALL TRUE)
        endif()

        add_subdirectory(${PKG_SOURCE_DIR} ${PKG_BINARY_DIR} EXCLUDE_FROM_ALL)

        if(ARG_DISABLE_INSTALL)
            set(CMAKE_SKIP_INSTALL_RULES FALSE)
            set(SKIP_INSTALL_ALL FALSE)
        endif()

        return()
    endif()

    set(FETCH_ARGS
        GIT_REPOSITORY ${ARG_GIT_REPOSITORY}
        SOURCE_DIR     ${PKG_SOURCE_DIR}
    )

    if(ARG_GIT_TAG)
        list(APPEND FETCH_ARGS GIT_TAG ${ARG_GIT_TAG})
    endif()

    if(ARG_RECURSE_SUBMODULES)
        list(APPEND FETCH_ARGS GIT_SUBMODULES_RECURSE TRUE)
    elseif(NOT ARG_NO_SHALLOW)
        list(APPEND FETCH_ARGS GIT_SHALLOW TRUE)
    endif()

    FetchContent_Declare(${package_name} ${FETCH_ARGS})
    FetchContent_GetProperties(${package_name})

    if(NOT ${package_name_lower}_POPULATED)
        if(POLICY CMP0169)
            cmake_policy(PUSH)
            cmake_policy(SET CMP0169 OLD)
        endif()

        FetchContent_Populate(${package_name})

        if(POLICY CMP0169)
            cmake_policy(POP)
        endif()

        if(ARG_POST_FETCH_SCRIPT)
            set(SCRIPT_PATH "${${package_name_lower}_SOURCE_DIR}/${ARG_POST_FETCH_SCRIPT}")
            if(EXISTS "${SCRIPT_PATH}")
                message(STATUS "Running post-fetch script: ${ARG_POST_FETCH_SCRIPT}")
                execute_process(
                    COMMAND chmod +x ${SCRIPT_PATH}
                    WORKING_DIRECTORY ${${package_name_lower}_SOURCE_DIR}
                )
                execute_process(
                    COMMAND ${SCRIPT_PATH}
                    WORKING_DIRECTORY ${${package_name_lower}_SOURCE_DIR}
                    RESULT_VARIABLE SCRIPT_RESULT
                    OUTPUT_VARIABLE SCRIPT_OUTPUT
                    ERROR_VARIABLE SCRIPT_ERROR
                )
                if(NOT SCRIPT_RESULT EQUAL 0)
                    message(STATUS "Script output: ${SCRIPT_OUTPUT}")
                    message(STATUS "Script error: ${SCRIPT_ERROR}")
                    message(FATAL_ERROR "Post-fetch script failed: ${ARG_POST_FETCH_SCRIPT}")
                endif()
            else()
                message(WARNING "Post-fetch script not found: ${SCRIPT_PATH}")
            endif()
        endif()

        if(ARG_POST_FETCH_COMMANDS)
            foreach(cmd ${ARG_POST_FETCH_COMMANDS})
                message(STATUS "Running post-fetch command: ${cmd}")
                separate_arguments(cmd_list UNIX_COMMAND "${cmd}")
                execute_process(
                    COMMAND ${cmd_list}
                    WORKING_DIRECTORY ${${package_name_lower}_SOURCE_DIR}
                    RESULT_VARIABLE CMD_RESULT
                )
                if(NOT CMD_RESULT EQUAL 0)
                    message(FATAL_ERROR "Post-fetch command failed: ${cmd}")
                endif()
            endforeach()
        endif()

        if(ARG_POST_FETCH_SCRIPT OR ARG_POST_FETCH_COMMANDS)
            set(MARKER_FILE "${${package_name_lower}_SOURCE_DIR}/.postfetch_done")
            file(WRITE "${MARKER_FILE}" "Post-fetch operations completed")
        endif()

        if(ARG_DISABLE_INSTALL)
            set(CMAKE_SKIP_INSTALL_RULES TRUE)
            set(SKIP_INSTALL_ALL TRUE)
        endif()

        add_subdirectory(
            ${${package_name_lower}_SOURCE_DIR}
            ${PKG_BINARY_DIR}
            EXCLUDE_FROM_ALL
        )

        if(ARG_DISABLE_INSTALL)
            set(CMAKE_SKIP_INSTALL_RULES FALSE)
            set(SKIP_INSTALL_ALL FALSE)
        endif()
    endif()
endfunction()
