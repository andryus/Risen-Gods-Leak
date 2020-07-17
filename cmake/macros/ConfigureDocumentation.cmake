find_package(Doxygen)

option(BUILD_DOCUMENTATION "Build API documentation" 0)

if (BUILD_DOCUMENTATION)
    message("* Build documentation    : Yes")
else()
    message("* Build documentation    : No (default)")
endif()

if (BUILD_DOCUMENTATION)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Cannot create documentation without doxygen")
    endif()
    
    set(DOCU_DIR ${CMAKE_BINARY_DIR}/doc/)
    set(DOCU_ICON ${CMAKE_SOURCE_DIR}/doc/icon.png)
    set(DOCU_REV "${rev_date} (${rev_hash})")
    
    set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/doc/Doxyfile.in)
    set(DOXYFILE ${CMAKE_BINARY_DIR}/doc/Doxyfile)
    
    configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)
    
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
        WORKING_DIRECTORY ${DOCU_DIR}
        COMMENT "Generating documentation with doxygen"
        VERBATIM
    )
endif()
