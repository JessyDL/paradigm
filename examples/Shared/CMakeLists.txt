include(inc.txt) 
include(src.txt)

add_library(ex_shared STATIC ${INC} ${SRC})
add_library(paradigm::examples::shared ALIAS ex_shared)

target_link_libraries(ex_shared PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core)
    
target_compile_features(ex_shared PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(ex_shared PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
set_target_properties(ex_shared PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET ex_shared PROPERTY FOLDER "paradigm-engine/examples/lib")

target_include_directories(ex_shared PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)
set_target_output_directory(TARGET ex_shared)
