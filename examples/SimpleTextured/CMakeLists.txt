add_executable(ex_simple_textured main.cpp)
add_executable(paradigm::examples::simple_textured ALIAS ex_simple_textured)
target_link_libraries(ex_simple_textured PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core paradigm::examples::shared)
    
target_compile_features(ex_simple_textured PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(ex_simple_textured PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
set_target_properties(ex_simple_textured PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET ex_simple_textured PROPERTY FOLDER "paradigm-engine/examples")
set_target_output_directory(TARGET ex_simple_textured DIRECTORY "examples/SimpleTextured")

add_custom_command(TARGET ex_simple_textured PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:ex_simple_textured>)
