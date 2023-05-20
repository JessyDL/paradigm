set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_CLANG_TIDY "")

add_subdirectory(fmt)
set_property(TARGET fmt PROPERTY FOLDER "extern")

option(SPDLOG_FMT_EXTERNAL "" ON)
set(spdlog_definitions "{ \"TRACE\", \"DEBUG\", \"INFO\", \"WARNING\", \"ERROR\", \"CRITICAL\", \"OFF\" }")
add_definitions(-DSPDLOG_LEVEL_NAMES=${spdlog_definitions})
add_subdirectory(spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "extern")
remove_definitions(-DSPDLOG_LEVEL_NAMES)

set(GLI_TEST_ENABLE OFF)
add_subdirectory(gli)
# silence all warnings, gli has some signed/unsigned issues and some issue /w unused variables
# todo: replace gli
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(gli INTERFACE "-w")
endif()
set_property(TARGET gli PROPERTY FOLDER "extern")
set_property(TARGET gli_dummy PROPERTY FOLDER "extern")

if(PE_TESTS)
    add_subdirectory(litmus)
    set_property(TARGET litmus PROPERTY FOLDER "extern")
endif()

add_subdirectory(utfcpp)
set_property(TARGET utf8cpp PROPERTY FOLDER "extern")

if(NOT TARGET strtype)
    add_subdirectory(strtype)
endif()
set_property(TARGET strtype PROPERTY FOLDER "extern")

# setup cppfront which lacks a cmake integration
add_executable(cppfront_compiler "${CMAKE_CURRENT_SOURCE_DIR}/cppfront/source/cppfront.cpp")
target_include_directories(cppfront_compiler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cppfront/source")
set_target_properties(cppfront_compiler PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED TRUE OUTPUT_NAME cppfront EXPORT_NAME cppfront)

add_library(cppfront_interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/cppfront/include/cpp2util.h")
set_target_properties(cppfront_interface PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED TRUE)
target_include_directories(cppfront_interface SYSTEM INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cppfront/include>")
