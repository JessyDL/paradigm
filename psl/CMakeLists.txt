cmake_minimum_required(VERSION 3.11)
set(PROJECT_PSL psl CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_PSL})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
	skarupke
	GIT_REPOSITORY https://github.com/skarupke/flat_hash_map.git
)

FetchContent_GetProperties(skarupke)
if(NOT skarupke_POPULATED)
  FetchContent_Populate(skarupke)
endif()

FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 5.3.0 GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
	message(STATUS "fetching fmt...")
	FetchContent_Populate(fmt)
	message(STATUS "fmt fetched")
endif()

SET(SPDLOG_FMT_EXTERNAL ON)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.3.0 GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
	message(STATUS "fetching spdlog...")
	FetchContent_Populate(spdlog)
	message(STATUS "spdlog fetched")
endif()

FetchContent_Declare(utf8cpp GIT_REPOSITORY https://github.com/nemtrif/utfcpp.git GIT_TAG v2.3.6 GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_GetProperties(utf8cpp)
if(NOT utf8cpp_POPULATED)
	message(STATUS "fetching utf8cpp...")
	FetchContent_Populate(utf8cpp)
	message(STATUS "utf8cpp fetched")
endif()

if(USE_RANGEV)
	FetchContent_Declare(rangev GIT_REPOSITORY https://github.com/ericniebler/range-v3.git GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
	FetchContent_GetProperties(rangev)
	if(NOT rangev_POPULATED)
		message(STATUS "fetching range-v3...")
		FetchContent_Populate(rangev)
		message(STATUS "range-v3 fetched")
	endif()
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

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(VS_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()


add_library(${LOCAL_PROJECT} STATIC ${INC} ${SRC} ${NATVIS})

target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
		${utf8cpp_SOURCE_DIR}/source
		${spdlog_SOURCE_DIR}/include
		${rangev_SOURCE_DIR}/include
		${skarupke_SOURCE_DIR}/
)
if(USE_RANGEV)
target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
		${rangev_SOURCE_DIR}/include
)
endif()

if(LINUX)
	target_link_libraries(${LOCAL_PROJECT} PUBLIC libc++experimental.a)
endif()

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