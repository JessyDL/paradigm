cmake_minimum_required(VERSION 3.11)
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

target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.2.1
)

FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)
  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
  utf8cpp
  GIT_REPOSITORY https://github.com/nemtrif/utfcpp.git
  GIT_TAG        v2.3.6
)

FetchContent_GetProperties(utf8cpp)
if(NOT utf8cpp_POPULATED)
  FetchContent_Populate(utf8cpp)
  add_subdirectory(${utf8cpp_SOURCE_DIR} ${utf8cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(LINUX)
	target_link_libraries(${LOCAL_PROJECT} PUBLIC libc++experimental.a spdlog utf8cpp)
else()
	target_link_libraries(${LOCAL_PROJECT} PUBLIC utf8cpp spdlog)
endif()
set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")
target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PUBLIC ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_LIB})


#set_target_properties(${LOCAL_PROJECT} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "inc/stdafx_psl.h")
cotire(${LOCAL_PROJECT})