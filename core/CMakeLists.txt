cmake_minimum_required(VERSION 3.11)
set(PROJECT_CORE core CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_CORE})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(inc.txt)
include(src.txt)
include(FetchContent)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()
link_libraries(${WSI_LIB})


FetchContent_Declare(gli GIT_REPOSITORY https://github.com/g-truc/gli.git GIT_TAG 0.8.2.0 GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_GetProperties(gli)
if(NOT gli_POPULATED)
	message(STATUS "fetching gli...")
	FetchContent_Populate(gli)
	message(STATUS "gli fetched")
endif()

if(${VULKAN_STATIC})		
	link_directories(${VULKAN_LIB_PATH})
else()	
	FetchContent_Declare(volk GIT_REPOSITORY https://github.com/zeux/volk.git GIT_TAG e610db090ba1cea4714d4e06a72f7f352deeda67 GIT_PROGRESS TRUE)
	FetchContent_GetProperties(volk)
	if(NOT volk_POPULATED) 
		message(STATUS "fetching volk...")
		FetchContent_Populate(volk)
		project(volk_proj VERSION 1.0.0 LANGUAGES CXX)
		add_library(volk_proj STATIC ${volk_SOURCE_DIR}/volk.c)
		target_compile_options(volk_proj PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
		target_compile_features(volk_proj PRIVATE ${PE_COMPILER_FEATURES})
		set_target_properties(volk_proj PROPERTIES LINKER_LANGUAGE CXX)
		target_include_directories(volk_proj PUBLIC ${VULKAN_INC_PATH})
		message(STATUS "${VULKAN_INC_PATH}")
		message(STATUS "volk fetched")
	endif()

	message(STATUS "generating dynamic lib for vulkan")
	EXECUTE_PROCESS(WORKING_DIRECTORY "${volk_SOURCE_DIR}" COMMAND python "generate.py" "https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/c4e056d365472174471a243dfefbfe66a03564af/registry/vk.xml")
	message(STATUS "generated dynamic lib")
endif()

if(PARADIGM_CORE_EXECUTABLE)
	add_executable(${LOCAL_PROJECT} ${SRC} ${INC} ${NATVIS})
else()
	add_library(${LOCAL_PROJECT} STATIC ${SRC} ${INC} ${NATVIS})
endif()	


if(${PE_TESTS})
	set(TEST_LIBS Catch2)
endif()

if(${VULKAN_STATIC})	
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${VULKAN_LIB_NAME}  ${PROJECT_PSL} ${PROJECT_FORMAT} 
		${PROJECT_META} ${PROJECT_INTERCEPT} ${PE_DL_LIBS} ${TEST_LIBS})
else()
	target_link_libraries(${LOCAL_PROJECT} PUBLIC 
		${PROJECT_PSL} ${PROJECT_FORMAT} 
		${PROJECT_META} ${PROJECT_INTERCEPT} ${CMAKE_DL_LIBS} ${PE_DL_LIBS} ${TEST_LIBS} volk)
endif()

target_include_directories(${LOCAL_PROJECT}
	PUBLIC 
      ${CMAKE_CURRENT_SOURCE_DIR}/inc
		${VULKAN_INC_PATH}
		${volk_SOURCE_DIR}
	PRIVATE
		${gli_SOURCE_DIR}
		${gli_SOURCE_DIR}/external
		${WSI_INC}
)

if(WSI_LIB OR NOT WSI_LIB STREQUAL "")
	target_link_libraries(${LOCAL_PROJECT} PUBLIC WSI_LIB)
endif()

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(${LOCAL_PROJECT} PUBLIC ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})
if(MSVC)
	target_compile_options(${LOCAL_PROJECT} PUBLIC /experimental:external /external:I${gli_SOURCE_DIR} /external:W3)
endif()
if(PE_PCH AND NOT MSVC)
	set(PROJECT_CORE_PCH core_pch CACHE INTERNAL "")
	set(LOCAL_PCH_PROJECT ${PROJECT_CORE_PCH})
	project(${LOCAL_PCH_PROJECT} VERSION 1.0.0 LANGUAGES CXX)
	add_custom_target(${PROJECT_CORE_PCH} DEPENDS stdafx.h.pch)

	get_target_property(dirs ${LOCAL_PROJECT} INCLUDE_DIRECTORIES)
	foreach(dir ${dirs})
	  list(APPEND INCLUDE_DIRS_LOCAL_PROJECT -I${dir} ) 
	endforeach()
	list(APPEND INCLUDE_DIRS_LOCAL_PROJECT "-I${CMAKE_SOURCE_DIR}/psl/inc") 
	list(APPEND INCLUDE_DIRS_LOCAL_PROJECT "-I${CMAKE_SOURCE_DIR}/meta/inc") 
	list(APPEND INCLUDE_DIRS_LOCAL_PROJECT "-I${CMAKE_SOURCE_DIR}/format/inc") 
	list(APPEND INCLUDE_DIRS_LOCAL_PROJECT "-I${spdlog_SOURCE_DIR}/include") 
	list(APPEND INCLUDE_DIRS_LOCAL_PROJECT "-I${utf8cpp_SOURCE_DIR}/source ") 
	message(STATUS ${INCLUDE_DIRS_LOCAL_PROJECT})

	set_property(TARGET ${LOCAL_PCH_PROJECT} PROPERTY FOLDER "PCH")

	add_custom_command(OUTPUT stdafx.h.pch
							COMMAND ${CMAKE_CXX_COMPILER} -x c++-header ${CMAKE_CURRENT_SOURCE_DIR}/inc/stdafx.h -Xclang -emit-pch -o stdafx.h.pch ${PE_COMPILE_OPTIONS} -I${utf8cpp_SOURCE_DIR}/source ${INCLUDE_DIRS_LOCAL_PROJECT} -I${CMAKE_CURRENT_SOURCE_DIR}/inc -DPLATFORM_LINUX -DPLATFORM_X64 ${PE_DEFINES}
							DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/inc/stdafx.h
							IMPLICIT_DEPENDS  CXX ${CMAKE_CURRENT_SOURCE_DIR}/inc/stdafx.h
							WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
							VERBATIM)
							
	add_dependencies(${LOCAL_PCH_PROJECT} ${PROJECT_PSL_PCH})
	add_dependencies(${LOCAL_PROJECT} ${LOCAL_PCH_PROJECT})
	target_compile_options(${LOCAL_PROJECT} PUBLIC -include-pch ${CMAKE_CURRENT_BINARY_DIR}/stdafx.h.pch)
endif()


if(PE_PCH AND MSVC)
	set_target_properties(${LOCAL_PROJECT} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "inc/stdafx.h")
	cotire(${LOCAL_PROJECT})
endif()