
function(pe_copy_target_shared_objects target_name)
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${target_name}>
            $<TARGET_FILE_DIR:${target_name}>
            COMMAND_EXPAND_LISTS
        )
    else()
        cmake_path(RELATIVE_PATH 
            CMAKE_CURRENT_BINARY_DIR 
            BASE_DIRECTORY $<TARGET_FILE_DIR:${origin_target}>
            OUTPUT_VARIABLE TBB_RELPATH
        )

        set_target_properties(core PROPERTIES
            BUILD_RPATH "$ORIGIN:$ORIGIN/${TBB_RELPATH}"
        )
    endif()
endfunction()