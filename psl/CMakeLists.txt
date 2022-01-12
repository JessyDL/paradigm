cmake_minimum_required(VERSION 3.11)
set(PROJECT_PSL psl CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_PSL})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
	async
	GIT_REPOSITORY https://github.com/JessyDL/async.git
)

FetchContent_GetProperties(async)
if(NOT async_POPULATED)
  message(STATUS "fetching jessydl/async...")
  set(ASYNC_NAMESPACE psl::async)
  set(ASYNC_MINIMAL_INCLUDE true)
  FetchContent_Populate(async)
  message(STATUS "jessydl/async fetched")
endif()


FetchContent_Declare(
	skarupke
	GIT_REPOSITORY https://github.com/skarupke/flat_hash_map.git
)

FetchContent_GetProperties(skarupke)
if(NOT skarupke_POPULATED)
  FetchContent_Populate(skarupke)
endif()

FetchContent_Declare(utf8cpp GIT_REPOSITORY https://github.com/nemtrif/utfcpp.git GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_GetProperties(utf8cpp)
if(NOT utf8cpp_POPULATED)
	message(STATUS "fetching utf8cpp...")
	FetchContent_Populate(utf8cpp)
	message(STATUS "utf8cpp fetched")
endif()

if(PE_PCH AND NOT MSVC)
	set(PROJECT_PSL_PCH psl_pch CACHE INTERNAL "")
	set(LOCAL_PCH_PROJECT ${PROJECT_PSL_PCH})
	project(${LOCAL_PCH_PROJECT} VERSION 1.0.0 LANGUAGES CXX)
	add_custom_target(${PROJECT_PSL_PCH} DEPENDS stdafx_psl.h.pch)
	
	add_custom_command(OUTPUT stdafx_psl.h.pch
							COMMAND ${CMAKE_CXX_COMPILER} -x c++-header ${CMAKE_CURRENT_SOURCE_DIR}/inc/stdafx_psl.h -Xclang -emit-pch -o stdafx_psl.h.pch ${PE_COMPILE_OPTIONS} -I${utf8cpp_SOURCE_DIR}/source -I${CMAKE_CURRENT_SOURCE_DIR}/inc -DPLATFORM_LINUX -DPLATFORM_X64 ${PE_DEFINES}
							DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/inc/stdafx_psl.h
							IMPLICIT_DEPENDS  CXX ${CMAKE_CURRENT_SOURCE_DIR}/inc/stdafx_psl.h
							WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
							VERBATIM)

	set_property(TARGET ${PROJECT_PSL_PCH} PROPERTY FOLDER "PCH")
endif()

include(inc.txt) 
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc/psl" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

find_package (Threads)

add_library(${LOCAL_PROJECT} STATIC ${INC} ${SRC} ${NATVIS})
target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
		${utf8cpp_SOURCE_DIR}/source
		${skarupke_SOURCE_DIR}/
		${async_SOURCE_DIR}/inc
)
target_compile_definitions(${LOCAL_PROJECT} PUBLIC "SPDLOG_FMT_EXTERNAL")

target_link_libraries(${LOCAL_PROJECT} PUBLIC fmt::fmt ${CMAKE_THREAD_LIBS_INIT} spdlog)

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine")
target_compile_features(${LOCAL_PROJECT} PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(${LOCAL_PROJECT} PUBLIC ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_LIB})

if(PE_PCH)	
	if(MSVC)
		set_target_properties(${LOCAL_PROJECT} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "inc/stdafx_psl.h")
		cotire(${LOCAL_PROJECT})
	else()
		add_dependencies(${LOCAL_PROJECT} ${PROJECT_PSL_PCH})
		target_compile_options(${LOCAL_PROJECT} PUBLIC -include-pch ${CMAKE_CURRENT_BINARY_DIR}/stdafx_psl.h.pch)
	endif()
endif()