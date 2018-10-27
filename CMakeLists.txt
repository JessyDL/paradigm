# in tree builds are not allowed
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "ERROR: detected an in-tree build. please create a sub-directory and invoke cmake from there, or a location outside the project.")
endif()

# we use https://cmake.org/cmake/help/latest/module/FetchContent.html
cmake_minimum_required(VERSION 3.11)
set(PROJECT_PARADIGM paradigm CACHE INTERNAL "")
project(${PROJECT_PARADIGM} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(${CMAKE_INCLUDE_PATH}/pch.cmake)
include(${CMAKE_INCLUDE_PATH}/platform.cmake)
include(${CMAKE_INCLUDE_PATH}/vulkan.cmake)
include(${CMAKE_INCLUDE_PATH}/assimp.cmake)


set_property(GLOBAL PROPERTY USE_FOLDERS ON) 

message("using compiler $ENV{CXX}")
message("using compiler ${CMAKE_CXX_COMPILER_ID}")

set(PROJECT_COMPILER_FEATURES "cxx_std_17")

target_architecture(ARCHITECTURE)

set(PARADIGM_ROOT ${PROJECT_SOURCE_DIR})
set(PARADIGM_EXTERNAL_PATH ${PARADIGM_ROOT}/externals)
set(VULKAN_VERSION CACHE STRING "default")
set(VULKAN_ROOT CACHE STRING "")
if (MSVC_IDE)
    option(VS_ADD_NATIVE_VISUALIZERS "Configure project to use Visual Studio native visualizers" TRUE)
else()
	set(VS_ADD_NATIVE_VISUALIZERS FALSE CACHE INTERNAL "Native visualizers are Visual Studio extension" FORCE)
endif()

if (UNIX AND NOT APPLE AND NOT ANDROID)
    set(LINUX TRUE)
endif()

OPTION(USE_D2D_WSI "use Direct to Display (if supported)" OFF)
if(LINUX)
	OPTION(USE_WAYLAND_WSI "use Wayland" OFF)
	OPTION(USE_XCB_WSI "use XCB" ON)
else()
	set(USE_WAYLAND_WSI FALSE CACHE INTERNAL "wayland is only for Linux OS's" FORCE)
	set(USE_XCB_WSI FALSE CACHE INTERNAL "XCB is only for Linux OS's" FORCE)
endif()

if(WIN32)
	OPTION(USE_WIN32_WSI "use win32" ON)
else()
	set(USE_WIN32_WSI FALSE CACHE INTERNAL "win32 is only for Windows OS" FORCE)
endif()

set(BUILD_DIRECTORY "" CACHE PATH "target location where to build the project to")
if(BUILD_DIRECTORY STREQUAL "")
	set(BUILD_DIRECTORY "${CMAKE_BINARY_DIR}/builds/")
endif()


# set(BaseName “binary” CACHE STRING “BaseName chosen by the user at CMake configure time”)
# set_property(CACHE BaseName PROPERTY STRINGS binary octal decimal hexadecimal)

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    message(FATAL_ERROR "we require a 64bit build platform")
endif()

# ==================================================================================================
# general defines
# ==================================================================================================
set(PARADIGM_DEFINES -DUNICODE 
				-D_UNICODE
				-DNOMINMAX
				-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
				
add_definitions(${PARADIGM_DEFINES} )				

SET(CMAKE_CXX_FLAGS_RELEASE "-DRELEASE -DNDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")


# ==================================================================================================
# OS specific
# ==================================================================================================

set(SHLWAPI "")
set(RPCRT4 "")
if(WIN32)
	set(SHLWAPI Shlwapi.lib)
	set(RPCRT4 Rpcrt4.lib)
endif()


# platform definitions
if (WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
else (LINUX)
    add_definitions(-DPLATFORM_LINUX)
	
endif()
add_definitions(-DPLATFORM_${ARCHITECTURE})
# ==================================================================================================
# Setup output directories
# ==================================================================================================
if(ARCHITECTURE STREQUAL "X64")
	set(ARCHI "x64")
else()
	set(ARCHI ${ARCHITEXTURE})
endif()

file(MAKE_DIRECTORY ${BUILD_DIRECTORY}/release/${ARCHI})
file(MAKE_DIRECTORY ${BUILD_DIRECTORY}/release_dbginfo/${ARCHI})
file(MAKE_DIRECTORY ${BUILD_DIRECTORY}/debug/${ARCHI})

set(EXECUTABLE_OUTPUT_PATH ${BUILD_DIRECTORY}/${ARCHI}/bin CACHE INTERNAL "")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIRECTORY}/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_DIRECTORY}/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIRECTORY}/${ARCHI}/bin CACHE INTERNAL "")
set(EXECUTABLE_OUTPUT_PATH_RELEASE ${BUILD_DIRECTORY}/release/${ARCHI}/bin CACHE INTERNAL "")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${BUILD_DIRECTORY}/release/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BUILD_DIRECTORY}/release/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BUILD_DIRECTORY}/release/${ARCHI}/bin CACHE INTERNAL "")
set(EXECUTABLE_OUTPUT_PATH_RELWITHDEBINFO ${BUILD_DIRECTORY}/release_dbginfo/${ARCHI}/bin CACHE INTERNAL "")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BUILD_DIRECTORY}/release_dbginfo/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BUILD_DIRECTORY}/release_dbginfo/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BUILD_DIRECTORY}/release_dbginfo/${ARCHI}/bin CACHE INTERNAL "")
set(EXECUTABLE_OUTPUT_PATH_DEBUG ${BUILD_DIRECTORY}/debug/${ARCHI}/bin CACHE INTERNAL "")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${BUILD_DIRECTORY}/debug/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BUILD_DIRECTORY}/debug/${ARCHI}/lib CACHE INTERNAL "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BUILD_DIRECTORY}/debug/${ARCHI}/bin CACHE INTERNAL "")

# ==================================================================================================
# figure out the generators
# ==================================================================================================
if(MSVC)
	if(CMAKE_GENERATOR_TOOLSET MATCHES ".*(clang|llvm|CLang|LLVM).*")
		set(GENERATOR_TOOLSET_VC false)	
	else()
		set(GENERATOR_TOOLSET_VC true)
	endif()
else()
	set(GENERATOR_TOOLSET_VC false)	
endif()

# ==================================================================================================
# General compiler & linker flags
# ==================================================================================================
set(CMAKE_CXX_EXTENSIONS OFF )
set(CXX_EXTENSIONS OFF )
if(GENERATOR_TOOLSET_VC)
	set(COMPILE_OPTIONS ${COMPILE_OPTIONS} /nologo /MP /sdl /Zc:forScope /Zc:inline /Zc:rvalueCast /Zc:wchar_t /GR- /fp:precise /W3 )
	set(COMPILE_OPTIONS_EXE ${COMPILE_OPTIONS_EXE}
							$<$<CONFIG:Debug>:             /MDd /Od /RTC1 /Z7>
							$<$<CONFIG:Release>:           /MD  /O2 /Oi /Gy /GL /Zi>
							$<$<CONFIG:RelWithDebInfo>:    /MD  /O2 /Oi /Gy /GL /Zo /Z7>
							$<$<CONFIG:RelNoOptimization>: /MDd /Od /RTC1 /Zi>)
	set(COMPILE_OPTIONS_LIB  ${COMPILE_OPTIONS_LIB}
							$<$<CONFIG:Debug>:             /MDd /Od /RTC1 /Z7>
							$<$<CONFIG:Release>:           /MD  /O2 /Oi /Gy /GL /Zi>
							$<$<CONFIG:RelWithDebInfo>:    /MD  /O2 /Oi /Gy /GL /Zo /Z7>
							$<$<CONFIG:RelNoOptimization>: /MDd /Od /RTC1 /Zi>)
	
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL /DEBUG /OPT:NOREF /OPT:NOICF")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /INCREMENTAL /DEBUG /OPT:NOREF /OPT:NOICF /LTCG:incremental")
else()
	set(COMPILE_OPTIONS ${COMPILE_OPTIONS} " -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function -std=c++1z")

	if (LINUX)
		set(COMPILE_OPTIONS "${COMPILE_OPTIONS} -stdlib=libc++")
	endif()

	# Add colors to ninja builds
	if (UNIX AND CMAKE_GENERATOR STREQUAL "Ninja")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
	endif()
	
	
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -fno-exceptions -fno-rtti)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer -ffunction-sections -fdata-sections")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2")
	
	# Strip unused sections
	set(GC_SECTIONS "--gc-sections")
	set(B_SYMBOLIC_FUNCTIONS "-Bsymbolic-functions")

	if (APPLE)
		set(GC_SECTIONS "-dead_strip")
		set(B_SYMBOLIC_FUNCTIONS "")

		# tell ranlib to ignore empty compilation units
		set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols <TARGET>")
		set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols <TARGET>")
		# prevents ar from invoking ranlib, let CMake do it
		set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> qc -S <TARGET> <LINK_FLAGS> <OBJECTS>")
		set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qc -S <TARGET> <LINK_FLAGS> <OBJECTS>")
	endif()
	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -g")
	set(COMPILE_OPTIONS "")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -Wl,${GC_SECTIONS} -g")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,${GC_SECTIONS} -Wl,${B_SYMBOLIC_FUNCTIONS}")
    
endif()

if(NOT ${VULKAN_STATIC})
	message("binding external volk library to handle dynamic bindings for vulkan")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_NO_PROTOTYPES -DVK_DYNAMIC_DISPATCH_VOLK")	
endif()

# ==================================================================================================
# Setup external dependencies
# ==================================================================================================
set(WSI_LIB "" CACHE INTERNAL "WSI library to link (if any)" FORCE)
set(WSI_INC "" CACHE INTERNAL "WSI include directories (if any)" FORCE)

if(USE_D2D_WSI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSURFACE_D2D")
	message("using D2D WSI")
elseif(USE_WAYLAND_WSI)
	find_package(Wayland REQUIRED)
	if (NOT WAYLAND_FOUND)
		message(FATAL_ERROR "Wayland development package not found")
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSURFACE_WAYLAND -DVK_USE_PLATFORM_WAYLAND_KHR")
	set(WSI_LIB ${WAYLAND_CLIENT_LIBRARIES})
	set(WSI_INC ${WAYLAND_INCLUDE_DIR})
	message("using Wayland WSI")
elseif(USE_XCB_WSI)
	find_package(XCB REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSURFACE_XCB -DVK_USE_PLATFORM_XCB_KHR")
	set(WSI_LIB ${XCB_LIBRARIES})
	message("using XCB WSI")
elseif(USE_WIN32_WSI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSURFACE_WIN32 -DVK_USE_PLATFORM_WIN32_KHR")
	message("using WIN32 WSI")
else()
	message(FATAL_ERROR "no surface to render set, or platform is not supported")
endif()


setup_vulkan("${VULKAN_ROOT}" "${VULKAN_VERSION}" VULKAN_INC_PATH VULKAN_LIB_PATH VULKAN_LIB_NAME)

#externals

#core libraries
add_subdirectory(psl)
add_subdirectory(format)
add_subdirectory(meta)
add_subdirectory(core)


# ==================================================================================================
# Set accessible defines for child projects to use
# ==================================================================================================

set(PARADIGM_DEFINES ${PARADIGM_DEFINES} CACHE STRING "Paradigm Defines")
set(PARADIGM_WSI_LIB ${WSI_LIB} CACHE STRING "The used WSI library define")
set(PARADIGM_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING "CXX flags")
set(PARADIGM_CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} CACHE STRING "CXX flags release")
set(PARADIGM_CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} CACHE STRING "CXX flags debug")
set(PARADIGM_CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} CACHE STRING "linker flags exe")
set(PARADIGM_CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} CACHE STRING "linker flags lib")
set(PARADIGM_COMPILE_OPTIONS ${COMPILE_OPTIONS} CACHE STRING "compile options")
set(PARADIGM_COMPILE_OPTIONS_EXE ${COMPILE_OPTIONS_EXE} CACHE STRING "compile options exe")
set(PARADIGM_COMPILE_OPTIONS_LIB  ${COMPILE_OPTIONS_LIB} CACHE STRING "compile options lib")