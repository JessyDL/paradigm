include(inc.txt)
include(src.txt)
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

SET(Python_ADDITIONAL_VERSIONS 3 3.6 3.5 3.4 3.3 3.2 3.1 3.0)
find_package(PythonInterp REQUIRED)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${GEN}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc/fwd" PREFIX "fwd" FILES ${FWD}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 	PREFIX "src" FILES ${SRC})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${INC_GLES}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 	PREFIX "src" FILES ${SRC_GLES}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" 	PREFIX "inc" FILES ${INC_VULKAN}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 	PREFIX "src" FILES ${SRC_VULKAN}) 

if(PE_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()
link_libraries(${WSI_LIB})

if(PE_VULKAN)
	FetchContent_Declare(vulkan GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers GIT_TAG v${PE_VULKAN_VERSION} GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
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

if(PE_GLES)
	if(PE_PLATFORM STREQUAL "WINDOWS")
		FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v0.1.36 GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
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
			if(PE_SURFACE STREQUAL "WIN32")
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
			
				list(APPEND GL_SOURCES
					"${GL_INCLUDE_DIRS}/src/glad_wgl.c"
					"${GL_INCLUDE_DIRS}/include/glad/glad_wgl.h"
				)				
				set_source_files_properties("${GL_INCLUDE_DIRS}/src/glad_wgl.c" PROPERTIES LANGUAGE CXX)
			endif()

			add_library(gl ${GL_SOURCES})
			set_target_properties(gl PROPERTIES LINKER_LANGUAGE CXX)
			target_include_directories(gl PUBLIC ${GL_INCLUDE_DIRS}/include)
			set_source_files_properties("${GL_INCLUDE_DIRS}/src/glad.c" PROPERTIES LANGUAGE CXX)
			set_property(TARGET gl PROPERTY FOLDER "extern")
			
			message(STATUS "Dav1dde/glad fetched")

			list(APPEND PE_DL_LIBS gl)
		endif()
	endif()
	if(PE_PLATFORM STREQUAL "LINUX")
		list(APPEND PE_DL_LIBS GLESv2 EGL)
	endif()
endif()

add_custom_target(core_generator)
add_custom_command(TARGET core_generator
	COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/generate_project_info.py
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../tools/
	BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/inc/paradigm.hpp
	COMMENT "Generating headers for Paradigm")

add_library(core STATIC ${FWD} ${SRC} ${INC} ${GEN} ${INC_GLES} ${SRC_GLES} ${INC_VULKAN} ${SRC_VULKAN} ${NATVIS})
add_library(paradigm::core ALIAS core)

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
set_target_output_directory(core)
target_include_directories(core
	PUBLIC 
      ${CMAKE_CURRENT_SOURCE_DIR}/inc
		${vulkan_SOURCE_DIR}/include
		$<IF:$<STREQUAL:"${PE_PLATFORM}","ANDROID">,${ANDROID_NDK}/sources/android/native_app_glue,>
	PRIVATE
		${WSI_INC}
)

target_link_libraries(core PUBLIC gli)

if(WSI_LIB OR NOT WSI_LIB STREQUAL "")
	target_link_libraries(core PUBLIC ${WSI_LIB})
endif()

set_property(TARGET core PROPERTY FOLDER "paradigm-engine")

target_compile_features(core PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(core PUBLIC ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
if(MSVC)
	target_compile_options(core PUBLIC /experimental:external /external:I${gli_SOURCE_DIR} /external:W3)
endif()

if(PE_MAKE_EXE)
	add_subdirectory(main)
endif()
