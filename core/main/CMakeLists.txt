cmake_minimum_required(VERSION 3.11)
set(PROJECT_MAIN main CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_MAIN})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

add_definitions(${PE_DEFINES})
add_executable(${LOCAL_PROJECT} main.cpp)

target_link_libraries(${LOCAL_PROJECT} PUBLIC 
    ${PROJECT_PSL} ${PE_DL_LIBS} ${PROJECT_CORE})
    
target_compile_features(${LOCAL_PROJECT} PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")