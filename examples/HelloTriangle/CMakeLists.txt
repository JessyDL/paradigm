add_executable(ex_hello_triangle main.cpp)
add_executable(paradigm::examples::hello_triangle ALIAS ex_hello_triangle)
target_link_libraries(ex_hello_triangle PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core)
    
target_compile_features(ex_hello_triangle PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(ex_hello_triangle PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
set_target_properties(ex_hello_triangle PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET ex_hello_triangle PROPERTY FOLDER "paradigm-engine/examples")
