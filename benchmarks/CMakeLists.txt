cmake_minimum_required(VERSION 3.10)
set(PROJECT_PARADIGM_BENCHMARKS benchmarks CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_PARADIGM_BENCHMARKS})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(inc.txt)
include(src.txt)
include(FetchContent)

FetchContent_Declare(
  google_benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.6.2
)

# todo: 1.7.0 of google benchmarks has a fatal compile issue, while the main branch doesn't work with C++23, so for the time being we need to silence this.
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

FetchContent_GetProperties(google_benchmark)
if(NOT google_benchmark_POPULATED)
  message(STATUS "fetching google/benchmark...")
  set(BENCHMARK_ENABLE_TESTING OFF)
  FetchContent_Populate(google_benchmark)
  add_subdirectory(${google_benchmark_SOURCE_DIR} ${google_benchmark_BINARY_DIR})
  message(STATUS "google/benchmark fetched")
endif()


set_property(TARGET benchmark PROPERTY FOLDER "extern")
set_property(TARGET benchmark_main PROPERTY FOLDER "extern")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

add_executable(${LOCAL_PROJECT} ${INC} ${SRC} ${NATVIS})

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "benchmarks")
target_link_libraries(${LOCAL_PROJECT} PUBLIC ${SHLWAPI} ${PROJECT_CORE} benchmark::benchmark)

target_include_directories(${LOCAL_PROJECT}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_EXE})