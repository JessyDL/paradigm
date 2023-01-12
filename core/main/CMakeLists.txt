if(PE_PLATFORM STREQUAL "ANDROID")
    add_subdirectory(android_native_app_glue)
    add_library(main SHARED main.cpp)
    add_library(paradigm::main ALIAS main)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    # todo: for some reason this is ignored?
    # target_link_options(${LOCAL_PROJECT} PRIVATE "-u ANativeActivity_onCreate")

    target_include_directories(main PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
    target_link_libraries(main PUBLIC paradigm::psl paradigm::core ${PE_DL_LIBS} PRIVATE android paradigm::android_native_glue EGL GLESv1_CM log)
else()
    add_executable(main main.cpp)
    add_executable(paradigm::main ALIAS main)
    target_link_libraries(main PUBLIC paradigm::psl ${PE_DL_LIBS} paradigm::core)
endif()
    
target_compile_features(main PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(main PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
set_target_properties(main PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET main PROPERTY FOLDER "paradigm-engine")

set_target_output_directory(main)
