cmake_minimum_required(VERSION 3.11)
set(PROJECT_CORE core CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_CORE})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(inc.txt)
include(src.txt)
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

SET(Python_ADDITIONAL_VERSIONS 3 3.6 3.5 3.4 3.3 3.2 3.1 3.0)
find_package(PythonInterp REQUIRED)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc/fwd" PREFIX "fwd" FILES ${FWD}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC_GLES}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC_GLES}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC_VULKAN}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC_VULKAN}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()
link_libraries(${WSI_LIB})

if(PE_VULKAN)
	FetchContent_Declare(vulkan GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers GIT_TAG v${VK_VERSION} GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
	FetchContent_GetProperties(vulkan)
	if(NOT vulkan_POPULATED)
		message(STATUS "fetching KhronosGroup/Vulkan-Headers...")
		FetchContent_Populate(vulkan)
		if(VK_STATIC)	
			add_subdirectory(${vulkan_SOURCE_DIR} ${vulkan_BINARY_DIR})
		endif()
		message(STATUS "KhronosGroup/Vulkan-Headers fetched")
	endif()
endif()

FetchContent_Declare(gli GIT_REPOSITORY https://github.com/g-truc/gli.git GIT_TAG 0.8.2.0 GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_GetProperties(gli)
if(NOT gli_POPULATED)
	message(STATUS "fetching g-truc/gli...")
	FetchContent_Populate(gli)
	message(STATUS "g-truc/gli fetched")
endif()

if(PE_VULKAN)
	if(VK_STATIC)
	
	else()	
		FetchContent_Declare(volk GIT_REPOSITORY https://github.com/zeux/volk.git GIT_PROGRESS TRUE)
		FetchContent_GetProperties(volk)
		if(NOT volk_POPULATED) 
			message(STATUS "fetching volk...")
			FetchContent_Populate(volk)
			
			message(STATUS "generating dynamic lib for vulkan")
			EXECUTE_PROCESS(WORKING_DIRECTORY "${volk_SOURCE_DIR}" COMMAND ${PYTHON_EXECUTABLE} "generate.py" ${vulkan_SOURCE_DIR}/registry/vk.xml)
			message(STATUS "generated dynamic lib")
			
			set_source_files_properties("${volk_SOURCE_DIR}/volk.c" PROPERTIES LANGUAGE CXX )
			
			project(volk_proj VERSION 1.0.0 LANGUAGES CXX)
			add_library(volk_proj STATIC ${volk_SOURCE_DIR}/volk.h ${volk_SOURCE_DIR}/volk.c)
			target_compile_options(volk_proj PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_LIB})
			target_compile_features(volk_proj PRIVATE ${PE_COMPILER_FEATURES})
			set_target_properties(volk_proj PROPERTIES LINKER_LANGUAGE CXX)
			target_include_directories(volk_proj PUBLIC ${vulkan_SOURCE_DIR}/include)
			message(STATUS "volk fetched")
		endif()	
	endif()
endif()

if(PE_GLES)
	FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v0.1.33 GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
	FetchContent_GetProperties(glad)
	if(NOT glad_POPULATED)
		message(STATUS "fetching Dav1dde/glad...")
		FetchContent_Populate(glad)
		set(GL_INCLUDE_DIRS "${glad_SOURCE_DIR}/out")
		
		list(APPEND GL_SOURCES
			"${GL_INCLUDE_DIRS}/src/glad.c"
			"${GL_INCLUDE_DIRS}/include/glad/glad.h"
		)
		EXECUTE_PROCESS(
			COMMAND ${PYTHON_EXECUTABLE} -m glad
				--profile=core
				--out-path=${GL_INCLUDE_DIRS}
				--api=gles2=3.2
				--generator=c
				--extensions=GL_EXT_sRGB,GL_EXT_texture_buffer,GL_EXT_texture_compression_s3tc,GL_EXT_texture_compression_s3tc_srgb,GL_EXT_texture_filter_anisotropic,GL_EXT_texture_filter_minmax,GL_KHR_debug
				--spec=gl
				--reproducible
			WORKING_DIRECTORY ${glad_SOURCE_DIR}
		)		
		
		list(APPEND GL_SOURCES
			"${GL_INCLUDE_DIRS}/src/glad_wgl.c"
			"${GL_INCLUDE_DIRS}/include/glad/glad_wgl.h"
		)
			
		EXECUTE_PROCESS(
			COMMAND ${PYTHON_EXECUTABLE} -m glad
				--out-path=${GL_INCLUDE_DIRS}
				--api=wgl=1.0
				--generator=c
				--extensions=WGL_ARB_create_context,WGL_ARB_create_context_profile,WGL_ARB_extensions_string,WGL_EXT_extensions_string,WGL_ARB_framebuffer_sRGB,WGL_ARB_multisample,WGL_EXT_create_context_es2_profile,WGL_EXT_pixel_format,WGL_EXT_swap_control
				--spec=wgl
				--omit-khrplatform
				--reproducible
			WORKING_DIRECTORY ${glad_SOURCE_DIR}
		)
		add_library(gl ${GL_SOURCES})
		set_target_properties(gl PROPERTIES LINKER_LANGUAGE CXX)		
		target_include_directories(gl PUBLIC ${GL_INCLUDE_DIRS}/include)
		set_source_files_properties("${GL_INCLUDE_DIRS}/src/glad_wgl.c" "${GL_INCLUDE_DIRS}/src/glad.c" PROPERTIES LANGUAGE CXX)
		
		message(STATUS "Dav1dde/glad fetched")
	endif()
endif()
if(PE_MODE STREQUAL "EXE")
	message("Core as executable")
	add_executable(${LOCAL_PROJECT} ${FWD} ${SRC} ${INC} ${INC_GLES} ${SRC_GLES} ${INC_VULKAN} ${SRC_VULKAN} ${NATVIS})
else()
	message("Core as library")
	add_library(${LOCAL_PROJECT} STATIC ${FWD} ${SRC} ${INC} ${INC_GLES} ${SRC_GLES} ${INC_VULKAN} ${SRC_VULKAN} ${NATVIS})
endif()	
if(PE_GLES)	
	target_link_libraries(${LOCAL_PROJECT} PUBLIC gl)
endif()
if(VK_STATIC AND PE_VULKAN)	
	if(WIN32)
		set(vk_lib_name "vulkan-1")
	else()
		set(vk_lib_name "libvulkan.so")
	endif()
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${vk_lib_name}  ${PROJECT_PSL} 
		${PROJECT_INTERCEPT} ${PE_DL_LIBS} ${TEST_LIBS})
else()
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${PROJECT_PSL} 
		${PROJECT_INTERCEPT} ${CMAKE_DL_LIBS} ${PE_DL_LIBS} ${TEST_LIBS} volk_proj)
endif()

target_include_directories(${LOCAL_PROJECT}
	PUBLIC 
      ${CMAKE_CURRENT_SOURCE_DIR}/inc
		${vulkan_SOURCE_DIR}/include
		${volk_SOURCE_DIR}
	PRIVATE
		${gli_SOURCE_DIR}
		${gli_SOURCE_DIR}/external/glm
		${WSI_INC}
)

if(WSI_LIB OR NOT WSI_LIB STREQUAL "")
	target_link_libraries(${LOCAL_PROJECT} PUBLIC ${WSI_LIB})
endif()

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(${LOCAL_PROJECT} PUBLIC ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
if(MSVC)
	target_compile_options(${LOCAL_PROJECT} PUBLIC /experimental:external /external:I${gli_SOURCE_DIR} /external:W3)
endif()

if(MSVC)
	#set_property(SOURCE "src/core.cpp" APPEND_STRING PROPERTY COMPILE_FLAGS " /bigobj ")
endif()