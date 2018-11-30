# in tree builds are not allowed
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "ERROR: detected an in-tree build. please create a sub-directory and invoke cmake from there, or a location outside the project.")
endif()

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    message(FATAL_ERROR "we require a 64bit build platform")
endif()

cmake_minimum_required(VERSION 3.11)
set(PROJECT_PARADIGM paradigm CACHE INTERNAL "")
project(${PROJECT_PARADIGM} VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CMakeDependentOption)

if (UNIX AND NOT APPLE AND NOT ANDROID)
    set(LINUX TRUE)
endif()

###############################################################################
###                    general options and setup                            ###
###############################################################################
OPTION(PE_PCH "generate and use precompiled headers (if supported)" OFF)
OPTION(PE_SOURCE_ONLY "set to off to not compile the sub-projects" OFF)
set(PE_MODE "LIB" CACHE STRING "should the core library be compiled as library (default), executable, or should the tests be built.")
set_property(CACHE PE_MODE PROPERTY STRINGS LIB EXE)
OPTION(PE_TESTS "should the tests be compiled? Note that, you cannot have core be an executable in that scenario" OFF)
OPTION(PE_PROFILER "enables the internal profiler" OFF)
OPTION(PE_VERBOSE "verbose error checking in the default compile options" OFF)
set(VK_ROOT "" CACHE PATH "The location root of your vulkan SDK (excluding the version folder)")
set(VK_VERSION "" CACHE STRING "The vulkan version to use")
OPTION(VK_STATIC "Should we statically bind Vulkan (if possible)" OFF)

include(FindXCB)
include(FindWayland)
OPTION(PE_WSI_D2D "use Direct to Display" OFF)
CMAKE_DEPENDENT_OPTION(PE_WSI_WAYLAND "use Wayland" ON "Wayland_FOUND" OFF)
CMAKE_DEPENDENT_OPTION(PE_WSI_XCB "use XCB" ON "XCB_FOUND" OFF)
CMAKE_DEPENDENT_OPTION(PE_WSI_WIN32 "use win32 surface" ON "WIN32" OFF)

set(PE_PLATFORM "AUTO" CACHE STRING "Target platform to build for, leave to 'AUTO' to detect and select the platform for you.")
set_property(CACHE PE_PLATFORM PROPERTY STRINGS AUTO WINDOWS LINUX ANDROID)
set(PE_INSTRUCTION_SET "OFF" CACHE STRING "Instruction set to use for SIMD instructions")
set_property(CACHE PE_INSTRUCTION_SET PROPERTY STRINGS OFF SSE2)

OPTION(VS_USE_NATVIS "Configure project to use Visual Studio native visualizers when building for visual studio" TRUE)

set(PE_BUILD_DIR "${CMAKE_BINARY_DIR}/builds/" CACHE PATH "target location where to build the project to")
set(PE_DEFINES -DUNICODE;-D_UNICODE;-DNOMINMAX CACHE INTERNAL "")
OPTION(PE_DEFAULT_COMPILE_OPTIONS "Use the default set of compile options (check 'compile options' section in the CMakelists.txt file)" TRUE)

if(PE_PCH)
	include(cotire)
endif()
include(vulkan)
include(assimp)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(PE_MODE STREQUAL "EXE")
	list(APPEND PE_DEFINES -DCORE_EXECUTABLE)
endif()

list(APPEND PE_DEFINES -DPLATFORM_${PE_PLATFORM})

if(NOT ${PE_INSTRUCTION_SET} STREQUAL "OFF")	
	list(APPEND PE_DEFINES -DINSTRUCTIONS_${PE_INSTRUCTION_SET})
endif()

if(${PE_PROFILER})
	list(APPEND PE_DEFINES -DPE_PROFILER)
endif()

if(NOT ${VK_STATIC} AND NOT ${PE_SOURCE_ONLY})	
	list(APPEND PE_DEFINES -DVK_NO_PROTOTYPES;-DVK_DYNAMIC_DISPATCH_VOLK)
endif()

###############################################################################
###                    validation                                           ###
###############################################################################

if(WIN32)
	list(APPEND PE_DL_LIBS Shlwapi.lib)
endif()

if(PE_MODE STREQUAL "EXE" AND PE_TESTS)
	message(FATAL_ERROR "you cannot mix [PE_EXE] and [PE_TESTS] flags")
endif()

###############################################################################
###                    setup output directories                             ###
###############################################################################
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PE_BUILD_DIR}/default/lib 
	CACHE PATH "Output directory for static libraries.")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PE_BUILD_DIR}/default/lib
	CACHE PATH "Output directory for shared libraries.")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PE_BUILD_DIR}/default/bin
	CACHE PATH "Output directory for executables and DLL's.")
	
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
file(MAKE_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
file(MAKE_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	string(TOLOWER ${OUTPUTCONFIG} OUTPUTCONFIG_FOLDERNAME)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/bin" CACHE PATH "" FORCE)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/lib" CACHE PATH "" FORCE)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/lib" CACHE PATH "" FORCE)
	
	file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG}}")
	file(MAKE_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG}}")
	file(MAKE_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG}}")
endforeach()

###############################################################################
###                    compile options                                      ###
###############################################################################

set(CMAKE_CXX_EXTENSIONS OFF )
set(CXX_EXTENSIONS OFF )
list(APPEND PE_COMPILER_FEATURES cxx_std_17)

if(MSVC) # we currently make no distinction for CLang (todo)
	list(APPEND PE_COMPILE_OPTIONS /nologo;/MP;/sdl;/permissive-;/Zc:forScope;/Zc:inline;/Zc:rvalueCast;/Zc:wchar_t;/fp:precise;/JMC)
	list(APPEND PE_COMPILE_OPTIONS /wd4505)
	if(${PE_VERBOSE})
		list(APPEND PE_COMPILE_OPTIONS /W4 /w14640 /w14242 /w14254)	
	else()
		list(APPEND PE_COMPILE_OPTIONS /W3)
	endif()
	
	list(APPEND PE_COMPILE_OPTIONS_EXE
							$<$<CONFIG:Debug>:             /MDd;/Od;/RTC1;/Z7>
							$<$<CONFIG:Release>:           /MD /O2;/Oi;/Gy;/GL;/Zi>
							$<$<CONFIG:RelWithDebInfo>:    /MD;/O2;/Oi;/Gy;/GL;/Zo;/Z7>
							$<$<CONFIG:RelNoOptimization>: /MDd;/Od;/RTC1;/Zi>)
	list(APPEND PE_COMPILE_OPTIONS_LIB
							$<$<CONFIG:Debug>:             /MDd;/Od;/RTC1;/Z7>
							$<$<CONFIG:Release>:           /MD;/O2;/Oi;/Gy;/Zi>
							$<$<CONFIG:RelWithDebInfo>:    /MD;/O2;/Oi;/Gy;/Zo;/Z7>
							$<$<CONFIG:RelNoOptimization>: /MDd;/Od;/RTC1;/Zi>)
	set(CMAKE_EXE_LINKER_FLAGS_$<$<CONFIG:Debug>:             /INCREMENTAL /DEBUG /OPT:NOREF /OPT:NOICF>
										$<$<CONFIG:Release>:           /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG>
										$<$<CONFIG:RelWithDebInfo>:    /INCREMENTAL /DEBUG /OPT:NOREF /OPT:NOICF /LTCG:incremental>
										$<$<CONFIG:RelNoOptimization>: /INCREMENTAL /OPT:NOREF /OPT:NOICF>)
else()
	list(APPEND PE_COMPILE_OPTIONS -fstrict-aliasing;-Wno-error=unused-command-line-argument;-Wno-unknown-pragmas;-Wno-unused-function;-std=c++1z;-g)
	list(APPEND PE_DL_LIBS c++fs)
	if (PE_PLATFORM STREQUAL "LINUX")
		list(APPEND PE_COMPILE_OPTIONS -stdlib=libc++)
	endif()
	
	if(${PE_VERBOSE})
		list(APPEND PE_COMPILE_OPTIONS -Wall;-Wextra)	
	endif()
	
	list(APPEND PE_COMPILE_OPTIONS
							$<$<CONFIG:Debug>:             -gdwarf-2>
							$<$<CONFIG:Release>:           -fomit-frame-pointer;-ffunction-sections;-fdata-sections>)
							
	# Add colors to ninja builds
	if (UNIX AND CMAKE_GENERATOR STREQUAL "Ninja")
		list(APPEND PE_COMPILE_OPTIONS -fcolor-diagnostics)
	endif()
	if (APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -dead_strip -g")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -dead_strip")
		
		# tell ranlib to ignore empty compilation units
		set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols <TARGET>")
		set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols <TARGET>")
		# prevents ar from invoking ranlib, let CMake do it
		set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> qc -S <TARGET> <LINK_FLAGS> <OBJECTS>")
		set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qc -S <TARGET> <LINK_FLAGS> <OBJECTS>")
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -dead_strip -g")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -dead_strip")
	endif()
endif()

###############################################################################
###                    setup WSI defines                                    ###
###############################################################################

set(HAS_WSI FALSE)
if(NOT HAS_WSI AND PE_WSI_D2D)
	list(APPEND PE_DEFINES -DSURFACE_D2D)
	message("using D2D WSI")
	set(HAS_WSI TRUE)
endif()
if(NOT HAS_WSI AND PE_WSI_WAYLAND AND WAYLAND_FOUND)
	list(APPEND PE_DEFINES -DSURFACE_WAYLAND;-DVK_USE_PLATFORM_WAYLAND_KHR)
	set(WSI_LIB ${WAYLAND_CLIENT_LIBRARIES})
	set(WSI_INC ${WAYLAND_INCLUDE_DIR})
	message("using Wayland WSI")
	set(HAS_WSI TRUE)
endif()
if(NOT HAS_WSI AND PE_WSI_XCB AND XCB_FOUND)
	list(APPEND PE_DEFINES -DSURFACE_XCB;-DVK_USE_PLATFORM_XCB_KHR)
	set(WSI_LIB ${XCB_LIBRARIES})
	message("using XCB WSI")
	set(HAS_WSI TRUE)
endif()
if(NOT HAS_WSI AND PE_WSI_WIN32)
	list(APPEND PE_DEFINES -DSURFACE_WIN32;-DVK_USE_PLATFORM_WIN32_KHR)
	message("using WIN32 WSI")
	set(HAS_WSI TRUE)
endif()
if(NOT HAS_WSI)
	message(FATAL_ERROR "Could not find a suitable WSI, or none was selected")
endif()

###############################################################################
###                    Add the sub projects                                 ###
###############################################################################

add_definitions(${PE_DEFINES})

if(NOT ${PE_SOURCE_ONLY})
	setup_vulkan("${VK_ROOT}" "${VK_VERSION}" VULKAN_INC_PATH VULKAN_LIB_PATH VULKAN_LIB_NAME)
	
	add_subdirectory(psl)
	add_subdirectory(core)
	if(${PE_TESTS})
		enable_testing()
		add_subdirectory(tests)
	endif()

	if(EXISTS "${PROJECT_SOURCE_DIR}/modules.txt")
		include(${PROJECT_SOURCE_DIR}/modules.txt)
	endif()
else()
	message("did not compile the project by request")
endif()
