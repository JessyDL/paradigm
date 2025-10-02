include(inc.txt)
include(src.txt)
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

find_package(Python3 REQUIRED)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/gen" 	PREFIX "inc" FILES ${GEN}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/fwd"     PREFIX "fwd" FILES ${FWD}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 	PREFIX "src" FILES ${SRC})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${INC_GLES}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 	PREFIX "src" FILES ${SRC_GLES}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${INC_VULKAN}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 	PREFIX "src" FILES ${SRC_VULKAN}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${INC_WEBGPU}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 	PREFIX "src" FILES ${SRC_WEBGPU}) 

if(PE_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()
link_libraries(${WSI_LIB})

if(PE_VULKAN)
	FetchContent_Populate(vulkan GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers GIT_TAG v${PE_VULKAN_VERSION} GIT_SHALLOW TRUE EXCLUDE_FROM_ALL)

	execute_process(
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/patch.py --includes ${vulkan_SOURCE_DIR}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
	)

	if(VK_STATIC)	
		add_subdirectory(${vulkan_SOURCE_DIR} ${vulkan_BINARY_DIR})
	endif()
endif()

if(PE_GLES)
	if(PE_PLATFORM STREQUAL "WINDOWS")
		FetchContent_Populate(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v2.0.8 GIT_SHALLOW TRUE EXCLUDE_FROM_ALL)

		message(STATUS "fetching Dav1dde/glad...")
		set(GL_INCLUDE_DIRS "${glad_SOURCE_DIR}/out")
		file(MAKE_DIRECTORY "${GL_INCLUDE_DIRS}")

		# make sure jinja2 is installed for python3
		execute_process(
			COMMAND ${Python3_EXECUTABLE} -m pip install jinja2 --user
			RESULT_VARIABLE JINJA2_RESULT
			OUTPUT_QUIET ERROR_QUIET
		)
			
		list(APPEND GL_SOURCES
			"${GL_INCLUDE_DIRS}/src/gles2.c"
			"${GL_INCLUDE_DIRS}/include/glad/gles2.h"
		)
		EXECUTE_PROCESS(
			COMMAND ${Python3_EXECUTABLE} -m glad
				--out-path=${GL_INCLUDE_DIRS}
				--api=gles2:core=3.2
				--extensions=GL_EXT_sRGB,GL_EXT_texture_buffer,GL_EXT_texture_compression_s3tc,GL_EXT_texture_compression_s3tc_srgb,GL_EXT_texture_filter_anisotropic,GL_EXT_texture_filter_minmax,GL_KHR_debug
				--reproducible
				c
			WORKING_DIRECTORY ${glad_SOURCE_DIR}
		)
		if(PE_SURFACE STREQUAL "WIN32")
			EXECUTE_PROCESS(
				COMMAND ${Python3_EXECUTABLE} -m glad
					--out-path=${GL_INCLUDE_DIRS}
					--api=wgl:core=1.0
					--extensions=WGL_ARB_create_context,WGL_ARB_create_context_profile,WGL_ARB_extensions_string,WGL_EXT_extensions_string,WGL_ARB_framebuffer_sRGB,WGL_ARB_multisample,WGL_EXT_create_context_es2_profile,WGL_EXT_pixel_format,WGL_EXT_swap_control
					--reproducible
					c
				WORKING_DIRECTORY ${glad_SOURCE_DIR}
			)

			# replace `#include <glad/gl.h>` with `#include <glad/gles2.h>` in wgl.h
			# by default the wgl expects the gl headers, but we are running gles on desktop
			EXECUTE_PROCESS(
				COMMAND ${Python3_EXECUTABLE} -c "import os; f = open(os.path.join('${GL_INCLUDE_DIRS}', 'include', 'glad', 'wgl.h'), 'r'); content = f.read(); f.close(); content = content.replace('#include <glad/gl.h>', '#include <glad/gles2.h>'); f = open(os.path.join('${GL_INCLUDE_DIRS}', 'include', 'glad', 'wgl.h'), 'w'); f.write(content); f.close()"
			)
			
			list(APPEND GL_SOURCES
				"${GL_INCLUDE_DIRS}/src/wgl.c"
				"${GL_INCLUDE_DIRS}/include/glad/wgl.h"
			)				
			set_source_files_properties("${GL_INCLUDE_DIRS}/src/glad.c" PROPERTIES LANGUAGE CXX)
		endif()

		add_library(gl STATIC ${GL_SOURCES})
		set_target_properties(gl PROPERTIES LINKER_LANGUAGE CXX)
		target_include_directories(gl PUBLIC ${GL_INCLUDE_DIRS}/include)
		set_source_files_properties("${GL_INCLUDE_DIRS}/src/glad.c" PROPERTIES LANGUAGE CXX)
			
		message(STATUS "Dav1dde/glad fetched")

		list(APPEND PE_DL_LIBS gl)
	endif()
	if(PE_PLATFORM STREQUAL "LINUX")
		list(APPEND PE_DL_LIBS GLESv2 EGL)
	endif()
endif()

add_custom_target(core_generator)
set_target_properties(core_generator PROPERTIES FOLDER "paradigm-engine/misc")
add_custom_command(TARGET core_generator PRE_BUILD
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/generate_project_info.py
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../tools/
	BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/gen/core/paradigm.hpp
	COMMENT "Generating headers for Paradigm")

add_library(core STATIC ${FWD} ${SRC} ${INC} ${GEN} ${INC_GLES} ${SRC_GLES} ${INC_VULKAN} ${SRC_VULKAN} ${INC_WEBGPU} ${SRC_WEBGPU} ${NATVIS})
add_library(paradigm::core ALIAS core)


if(MSVC)
	add_custom_target(core_patcher
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/patch.py --project ${CMAKE_BINARY_DIR}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
		COMMENT "Patching build files")
	add_dependencies(core core_patcher)
	set_target_properties(core_patcher PROPERTIES FOLDER "paradigm-engine/misc")
endif()

add_dependencies(core core_generator)

if(${PE_PCH})
	target_precompile_headers(core PUBLIC ${INC_PCH})
endif()

if(VK_STATIC AND PE_VULKAN)	
	if(WIN32)
		set(vk_lib_name "vulkan-1")
	else()
		set(vk_lib_name "libvulkan.so")
	endif()
	list(APPEND PE_DL_LIBS ${vk_lib_name})
endif()

target_link_libraries(core PUBLIC paradigm::psl ${CMAKE_DL_LIBS} ${PE_DL_LIBS} ${TEST_LIBS})
set_target_output_directory(TARGET core)
target_include_directories(core
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/inc
	${CMAKE_CURRENT_SOURCE_DIR}/fwd
	${CMAKE_CURRENT_SOURCE_DIR}/gen
		${vulkan_SOURCE_DIR}/include
		$<IF:$<STREQUAL:"${PE_PLATFORM}","ANDROID">,${ANDROID_NDK}/sources/android/native_app_glue,>
	PRIVATE
		${WSI_INC}
)

target_link_libraries(core 
	PRIVATE
		gli dr_libs
	PUBLIC
		$<$<BOOL:${PE_WEBGPU_NATIVE}>:webgpu_dawn webgpu_cpp>
)

if(WSI_LIB OR NOT WSI_LIB STREQUAL "")
	target_link_libraries(core PUBLIC ${WSI_LIB})
endif()

set_property(TARGET core PROPERTY FOLDER "paradigm-engine")
set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(psl PUBLIC ${PE_DEFINES})

target_compile_features(core PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(core PUBLIC ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
if(MSVC)
	target_compile_options(core PRIVATE /experimental:external /external:I${gli_SOURCE_DIR} /external:W3)
endif()

if(PE_MAKE_EXE)
	add_subdirectory(main)
endif()

install(TARGETS core)
