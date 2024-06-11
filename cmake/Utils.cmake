
macro(set_target_output_directory)
    set(oneValueArgs DIRECTORY)
    set(multiValueArgs TARGET)
    cmake_parse_arguments(SET_TARGET_OUTPUT_DIRECTORY "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT DEFINED ${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY})
        set(${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY} "")
    endif()
    foreach(targ ${SET_TARGET_OUTPUT_DIRECTORY_TARGET})
        get_target_property(target_type ${targ} TYPE)
        foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
            string(TOLOWER ${OUTPUTCONFIG} OUTPUTCONFIG_FOLDERNAME)
            # mostly done so the directories don't get created in the output for binaries when unneeded.
            if (target_type STREQUAL "EXECUTABLE")
                set_target_properties(${targ}
                    PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/bin/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}")
            else()
                set_target_properties(${targ}
                    PROPERTIES
                    ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/lib/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}"
                    LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/lib/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}"
                    RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/bin/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}")
            endif()
        endforeach()
        if (target_type STREQUAL "EXECUTABLE")
            set_target_properties(${targ}
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/bin/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}")
        else ()
            set_target_properties(${targ}
                PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/lib/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}"
                LIBRARY_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/lib/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}"
                RUNTIME_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/bin/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}")
        endif()
    endforeach()
endmacro()
