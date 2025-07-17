# todo: currently examples cannot diverge from the main project, this should be fixed in the future.
if(PE_CSHARP_BINDINGS)
    add_executable(ex_csharp_bindings main.cpp)
    add_executable(paradigm::examples::csharp_bindings ALIAS ex_csharp_bindings)
    target_link_libraries(ex_csharp_bindings PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core)
    
    target_compile_features(ex_csharp_bindings PUBLIC ${PE_COMPILER_FEATURES})
    target_compile_options(ex_csharp_bindings PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
    set_target_properties(ex_csharp_bindings PROPERTIES LINKER_LANGUAGE CXX)

    set_property(TARGET ex_csharp_bindings PROPERTY FOLDER "paradigm-engine/examples")
    set_target_output_directory(TARGET ex_csharp_bindings DIRECTORY "examples/CSharpBindings")
endif()