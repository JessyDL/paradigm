
add_executable(psl_main main.cpp)
add_executable(paradigm::psl::main ALIAS psl_main)

target_link_libraries(psl_main psl)
set_target_properties(psl_main PROPERTIES LINKER_LANGUAGE CXX)


set_target_properties(psl_main PROPERTIES FOLDER "paradigm-engine")
if(PE_PLATFORM STREQUAL "WEB")
    set_target_properties(psl_main PROPERTIES SUFFIX ".html")
endif()
target_compile_features(psl_main PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(psl_main PUBLIC ${PE_COMPILE_OPTIONS})
