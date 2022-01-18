cmake_minimum_required(VERSION 3.10)
set(PROJECT_PARADIGM_TEST tests CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_PARADIGM_TEST})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

# todo: support ctest with litmus
# include(CTest)
include(inc.txt)
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

add_executable(${LOCAL_PROJECT} ${INC} ${SRC} ${NATVIS})

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "tests")
target_link_libraries(${LOCAL_PROJECT} PUBLIC ${SHLWAPI} ${PROJECT_CORE} litmus)

target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_EXE})
