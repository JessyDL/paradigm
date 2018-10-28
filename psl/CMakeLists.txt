cmake_minimum_required(VERSION 3.10)
set(PROJECT_PSL psl CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_PSL})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(inc.txt)
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_ADD_NATIVE_VISUALIZERS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

add_library(${LOCAL_PROJECT} STATIC ${INC} ${SRC} ${NATVIS})

set(_PCH_SOURCE_CXX "src/stdafx_psl.cpp")
add_precompiled_header(${LOCAL_PROJECT} "stdafx_psl.h")

target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
		${PARADIGM_EXTERNAL_PATH}/inc/utf8
		${PARADIGM_EXTERNAL_PATH}/inc/spdlog
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(LINUX)
target_link_libraries(${LOCAL_PROJECT} PUBLIC 
	libc++experimental.a )
endif()
set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_LIB})