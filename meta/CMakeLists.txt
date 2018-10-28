cmake_minimum_required(VERSION 3.10)
set(PROJECT_META meta CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_META})
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

target_link_libraries(${LOCAL_PROJECT} PUBLIC ${PROJECT_PSL} ${PROJECT_FORMAT} ${SHLWAPI} ${RPCRT4})

if(LINUX)
link_directories("/usr/lib")
target_link_libraries(${LOCAL_PROJECT} PUBLIC 
	libuuid.so )
endif()

target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_LIB})