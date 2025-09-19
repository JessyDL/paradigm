add_executable(ex_instancing main.cpp)
add_executable(paradigm::examples::instancing ALIAS ex_instancing)
target_link_libraries(ex_instancing PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core paradigm::examples::shared)
    
target_compile_features(ex_instancing PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(ex_instancing PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
set_target_properties(ex_instancing PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET ex_instancing PROPERTY FOLDER "paradigm-engine/examples")
set_target_output_directory(TARGET ex_instancing DIRECTORY "examples/Instancing")
include(Utils)
assembler_generate_files(TARGET ex_instancing)
