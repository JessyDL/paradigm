
macro(set_target_output_directory)
    set(oneValueArgs DIRECTORY)
    set(multiValueArgs TARGET)
    cmake_parse_arguments(SET_TARGET_OUTPUT_DIRECTORY "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT DEFINED ${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY})
        set(${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY} "")
    endif()
    if(${PE_INTERNAL_SET_BUILD_DIR})
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
    endif()
endmacro()

macro(assembler_generate_files)
    set(oneValueArgs TARGET)
    cmake_parse_arguments(SET_ASSEMBLER_GENERATE_FILES "" ${oneValueArgs} "" ${ARGN})
    if(NOT SET_ASSEMBLER_GENERATE_FILES_TARGET)
        message(FATAL_ERROR "assembler_generate_files: TARGET argument is required")
    endif()

    add_dependencies(${SET_ASSEMBLER_GENERATE_FILES_TARGET} assembler)
    get_target_property(TARGET_SOURCE_DIR ${SET_ASSEMBLER_GENERATE_FILES_TARGET} SOURCE_DIR)

    add_custom_command(TARGET ${SET_ASSEMBLER_GENERATE_FILES_TARGET} PRE_BUILD
        COMMAND echo "Assembler generating files for '${SET_ASSEMBLER_GENERATE_FILES_TARGET}'"
        COMMAND $<TARGET_FILE:assembler> -g -p -i ${TARGET_SOURCE_DIR}/project.ppf -o $<TARGET_FILE_DIR:${SET_ASSEMBLER_GENERATE_FILES_TARGET}>/data
    )
endmacro()