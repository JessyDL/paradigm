cmake_minimum_required(VERSION 3.10)
set(PROJECT_CORE core CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_CORE})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(inc.txt)
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_ADD_NATIVE_VISUALIZERS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()
link_libraries(${WSI_LIB})


if(NOT ${VULKAN_STATIC})	
	link_directories(${VULKAN_LIB_PATH})

	source_group(TREE "${PARADIGM_EXTERNAL_PATH}/volk" PREFIX "volk" FILES ${PARADIGM_EXTERNAL_PATH}/volk/volk.c) 
	if(PARADIGM_CORE_EXECUTABLE)
		add_executable(${LOCAL_PROJECT} ${SRC} ${INC} ${NATVIS} ${PARADIGM_EXTERNAL_PATH}/volk/volk.c)
	else()
		add_library(${LOCAL_PROJECT} STATIC ${SRC} ${INC} ${NATVIS} ${PARADIGM_EXTERNAL_PATH}/volk/volk.c)
	endif()
	SET_SOURCE_FILES_PROPERTIES( ${PARADIGM_EXTERNAL_PATH}/volk/volk.c PROPERTIES LANGUAGE CXX )
	set(_PCH_EXCLUDED ${PARADIGM_EXTERNAL_PATH}/volk/volk.c)
else()
	if(PARADIGM_CORE_EXECUTABLE)
		add_executable(${LOCAL_PROJECT} ${SRC} ${INC} ${NATVIS})
	else()
		add_library(${LOCAL_PROJECT} STATIC ${SRC} ${INC} ${NATVIS})
	endif()
endif()

set(_PCH_SOURCE_CXX "src/stdafx.cpp")
add_precompiled_header(${LOCAL_PROJECT} "stdafx.h")

if(NOT ${VULKAN_STATIC})	
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${VULKAN_LIB_NAME}  ${PROJECT_PSL} ${PROJECT_FORMAT} 
		${PROJECT_META} ${PROJECT_INTERCEPT} ${SHLWAPI} )
else()
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${PROJECT_PSL} ${PROJECT_FORMAT} 
		${PROJECT_META} ${PROJECT_INTERCEPT} ${SHLWAPI} )
endif()

target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
		${VULKAN_INC_PATH}
		${PARADIGM_EXTERNAL_PATH}/inc/gli
		${PARADIGM_EXTERNAL_PATH}/inc/glm
	PRIVATE
		${WSI_INC}
)

if(LINUX)
	target_link_libraries(${LOCAL_PROJECT} PUBLIC "xcb")
endif()

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_EXE})