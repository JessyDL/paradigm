
macro(set_target_output_directory target)    
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        string(TOLOWER ${OUTPUTCONFIG} OUTPUTCONFIG_FOLDERNAME)
        set_target_properties(${target}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/lib"
            LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/lib"
            RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/bin")
    endforeach()
    set_target_properties(${target}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/lib"
        LIBRARY_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/lib"
        RUNTIME_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/bin")
endmacro()
