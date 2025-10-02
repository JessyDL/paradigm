
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
                        LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/bin/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}"
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
                    LIBRARY_OUTPUT_DIRECTORY "${PE_BUILD_DIR}/default/bin/${SET_TARGET_OUTPUT_DIRECTORY_DIRECTORY}"
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

macro(add_example directory_name)
    # transform the CamelCase directory_name to snake_case for the folder structure
    string(REGEX REPLACE "([a-z])([A-Z])" "\\1_\\2" snake_case_name ${directory_name})
    string(TOLOWER ${snake_case_name} target_name)
    set(ex_target_name "ex_${target_name}")
    
    add_executable(${ex_target_name} ${directory_name}/main.cpp)    
    add_executable(paradigm::examples::${target_name} ALIAS ${ex_target_name})
    target_link_libraries(${ex_target_name} PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core paradigm::examples::shared)
    
    target_compile_features(${ex_target_name} PUBLIC ${PE_COMPILER_FEATURES})
    target_compile_options(${ex_target_name} PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
    set_target_properties(${ex_target_name} PROPERTIES LINKER_LANGUAGE CXX FOLDER "paradigm-engine/examples")
    set_target_output_directory(TARGET ${ex_target_name} DIRECTORY "examples/${directory_name}")
    # if the project.ppf file exist, then run the assembler:
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${directory_name}/project.ppf")
        assembler_generate_files(TARGET ${ex_target_name})
    endif()
    pe_copy_target_shared_objects(${ex_target_name})
endmacro()

macro(get_all_cmake_targets result)
    set(${result})
    # Use execute_process or a simpler directory-based approach
    get_directory_targets(${result} ${CMAKE_BINARY_DIR})
endmacro()

function(get_directory_targets result dir)
    get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    set(all_targets ${targets})
    
    get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirs})
        get_directory_targets(subdir_targets ${subdir})
        list(APPEND all_targets ${subdir_targets})
    endforeach()
    
    set(${result} ${all_targets} PARENT_SCOPE)
endfunction()

function(reset_external_cmake_folders)
    get_directory_targets(all_targets ${CMAKE_SOURCE_DIR})

    foreach(target ${all_targets})
        if(TARGET ${target})
            get_target_property(current_folder ${target} FOLDER)
        
            if(NOT current_folder MATCHES "^paradigm\-engine")
                set_target_properties(${target} PROPERTIES FOLDER "paradigm-engine/external")
            endif()
        endif()
    endforeach()
endfunction()
