if(NOT DEFINED SOURCE_FILE OR NOT DEFINED DEST_FILE)
    message(FATAL_ERROR "SOURCE_FILE and DEST_FILE are required")
endif()

if(NOT EXISTS "${DEST_FILE}")
    get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${DEST_DIR}")
    file(COPY_FILE "${SOURCE_FILE}" "${DEST_FILE}")
endif()
