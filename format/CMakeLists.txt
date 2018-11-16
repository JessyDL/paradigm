cmake_minimum_required(VERSION 3.10)
set(PROJECT_FORMAT format CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_FORMAT})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)


include(inc.txt)
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()


add_library(${LOCAL_PROJECT} STATIC ${SRC} ${INC} ${NATVIS})

target_link_libraries(${LOCAL_PROJECT} PUBLIC ${PROJECT_PSL})

target_include_directories(${LOCAL_PROJECT}
	PUBLIC   
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_LIB})