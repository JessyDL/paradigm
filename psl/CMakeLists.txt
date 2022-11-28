cmake_minimum_required(VERSION 3.11)
set(PROJECT_PSL psl CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_PSL})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(inc.txt) 
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc/psl" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

find_package (Threads)

add_library(${LOCAL_PROJECT} STATIC ${INC} ${SRC} ${NATVIS})
target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)
target_compile_definitions(${LOCAL_PROJECT} PUBLIC "SPDLOG_FMT_EXTERNAL")

target_link_libraries(${LOCAL_PROJECT} PUBLIC fmt::fmt ${CMAKE_THREAD_LIBS_INIT} spdlog utf8cpp strtype)

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")
target_compile_features(${LOCAL_PROJECT} PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(${LOCAL_PROJECT} PUBLIC ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_LIB})
