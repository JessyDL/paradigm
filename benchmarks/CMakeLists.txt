include(inc.txt)
include(src.txt)
include(FetchContent)

FetchContent_Declare(
  google_benchmark
  GIT_REPOSITORY https://github.com/JessyDL/benchmark.git
  GIT_TAG        fix/win32_unicode_support
)

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

if(PE_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

add_executable(benchmarks ${INC} ${SRC} ${NATVIS})
add_executable(paradigm::benchmarks ALIAS benchmarks)

set_property(TARGET benchmarks PROPERTY FOLDER "benchmarks")
target_link_libraries(benchmarks PUBLIC ${SHLWAPI} paradigm::core benchmark::benchmark)
set_target_properties(benchmarks PROPERTIES LINKER_LANGUAGE CXX)

set_target_output_directory(benchmarks)
target_include_directories(benchmarks
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(benchmarks PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(benchmarks PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
