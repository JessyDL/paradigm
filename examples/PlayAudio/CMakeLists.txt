add_executable(ex_play_audio main.cpp)
add_executable(paradigm::examples::play_audio ALIAS ex_play_audio)
target_link_libraries(ex_play_audio PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core paradigm::examples::shared)
    
target_compile_features(ex_play_audio PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(ex_play_audio PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
set_target_properties(ex_play_audio PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET ex_play_audio PROPERTY FOLDER "paradigm-engine/examples")
set_target_output_directory(TARGET ex_play_audio DIRECTORY "examples/PlayAudio")

include(Utils)
assembler_generate_files(TARGET ex_play_audio)
