cmake_minimum_required(VERSION 3.11)
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

include(FetchContent)

FetchContent_Declare(
  gli
  GIT_REPOSITORY https://github.com/g-truc/gli.git
  GIT_TAG        0.8.2.0
)

FetchContent_GetProperties(gli)
if(NOT gli_POPULATED)
  FetchContent_Populate(gli)
  add_subdirectory(${gli_SOURCE_DIR} ${gli_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.8.5
)

FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
  FetchContent_Populate(glm)
  add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
  volk 
  GIT_REPOSITORY https://github.com/zeux/volk.git
  GIT_TAG        e610db090ba1cea4714d4e06a72f7f352deeda67
)

FetchContent_GetProperties(volk)
if(NOT volk_POPULATED)
  FetchContent_Populate(volk)
  add_subdirectory(${volk_SOURCE_DIR} ${volk_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

EXECUTE_PROCESS(WORKING_DIRECTORY "${volk_SOURCE_DIR}" COMMAND python "generate.py" "https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/c4e056d365472174471a243dfefbfe66a03564af/registry/vk.xml")

if(${VULKAN_STATIC})		
	link_directories(${VULKAN_LIB_PATH})
	if(PARADIGM_CORE_EXECUTABLE)
		add_executable(${LOCAL_PROJECT} ${SRC} ${INC} ${NATVIS})
	else()
		add_library(${LOCAL_PROJECT} STATIC ${SRC} ${INC} ${NATVIS})
	endif()	
else()	
	if(PARADIGM_CORE_EXECUTABLE)
		add_executable(${LOCAL_PROJECT} ${SRC} ${INC} ${NATVIS})
	else()
		add_library(${LOCAL_PROJECT} STATIC ${SRC} ${INC} ${NATVIS})
	endif()
endif()

set(_PCH_SOURCE_CXX "stdafx.cpp")
set(_PCH_SOURCE_PREFIX "src/")
add_precompiled_header(${LOCAL_PROJECT} "stdafx.h")

if(TESTS)
	set(TEST_LIBS Catch2)
endif()

if(${VULKAN_STATIC})	
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${VULKAN_LIB_NAME}  ${PROJECT_PSL} ${PROJECT_FORMAT} 
		${PROJECT_META} ${PROJECT_INTERCEPT} ${SHLWAPI} ${TEST_LIBS} gli glm volk)
else()
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${PROJECT_PSL} ${PROJECT_FORMAT} 
		${PROJECT_META} ${PROJECT_INTERCEPT} ${SHLWAPI} ${CMAKE_DL_LIBS} ${PARADIGM_DL_LIBS} ${TEST_LIBS} gli glm volk)
endif()

target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
      ${CMAKE_CURRENT_SOURCE_DIR}/inc
		${VULKAN_INC_PATH}
		${volk_SOURCE_DIR}
	PRIVATE
		${WSI_INC}
)

if(LINUX)
	target_link_libraries(${LOCAL_PROJECT} PUBLIC "xcb")
endif()

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_EXE})