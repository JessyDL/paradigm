
add_executable(psl_main main.cpp)
add_executable(paradigm::psl::main ALIAS psl_main)

list(APPEND PUBLIC_LIBRARIES psl)
list(APPEND PRIVATE_LIBRARIES)
if(${PE_WEBGPU_NATIVE})
    list(APPEND PRIVATE_LIBRARIES webgpu_cpp webgpu_dawn)
endif()
target_link_libraries(psl_main PUBLIC ${PUBLIC_LIBRARIES} PRIVATE ${PRIVATE_LIBRARIES})
set_target_properties(psl_main PROPERTIES LINKER_LANGUAGE CXX)


set_target_properties(psl_main PROPERTIES FOLDER "paradigm-engine")
if(PE_PLATFORM STREQUAL "WEB")
    set_target_properties(psl_main PROPERTIES SUFFIX ".html")
endif()
target_compile_features(psl_main PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(psl_main PUBLIC ${PE_COMPILE_OPTIONS})
