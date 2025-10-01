
function(pe_copy_target_shared_objects target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${target_name}>
        $<TARGET_FILE_DIR:${target_name}>
        COMMAND_EXPAND_LISTS
    )
endfunction()