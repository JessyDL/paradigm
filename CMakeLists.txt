# in tree builds are not allowed
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "ERROR: detected an in-tree build. please create a sub-directory and invoke cmake from there, or a location outside the project.")
endif()

cmake_minimum_required(VERSION 3.16)
set(PE_NAME paradigm CACHE INTERNAL "")
project(${PE_NAME} VERSION 1.0.0 LANGUAGES CXX C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CMakeDependentOption)


if (UNIX AND NOT APPLE AND NOT ANDROID)
    set(LINUX TRUE)
endif()

###############################################################################
###                    general options and setup                            ###
###############################################################################
OPTION(PE_PROFILER "enables the internal profiler" OFF)
OPTION(PE_ANALYZE "enables static analyzers for the given platform" OFF)
OPTION(PE_VERBOSE "verbose error checking in the default compile options" OFF)
set(PE_VULKAN_VERSION "1.2.148" CACHE STRING "The vulkan version to use")
OPTION(VK_STATIC "Should we statically bind Vulkan (if possible)" OFF)
OPTION(PE_GLES "controls OpenGL ES support" ON)
OPTION(PE_VULKAN "controls Vulkan support (experimental - not done)" ON)
OPTION(PE_MOLTEN "controls Metal support (not implemented)" OFF)
OPTION(PE_MAKE_EXE "make example executable based on Paradigm" ON)
OPTION(PE_DEV_MAKE_PSL_EXE "For development you can make an executable " OFF)
OPTION(PE_TESTS "make the tests" ON)
OPTION(PE_PCH "enable usage of the precompiled header" ON)
OPTION(PE_CCACHE "enable ccache" ON)
OPTION(PE_CORE "generate the paradigm core, turn this off if you only want the PSL to generate" ON)

include(Utils)
include(FindXCB)
OPTION(PE_WSI_D2D "use Direct to Display" OFF)
CMAKE_DEPENDENT_OPTION(PE_WSI_XCB "use XCB" ON "XCB_FOUND" OFF)
CMAKE_DEPENDENT_OPTION(PE_WSI_WIN32 "use win32 surface" ON "WIN32" OFF)

set(PE_PLATFORM "AUTO" CACHE STRING "Target platform to build for, leave to 'AUTO' to detect and select the platform for you.")
set_property(CACHE PE_PLATFORM PROPERTY STRINGS AUTO WINDOWS LINUX ANDROID MACOS WEB)
set(PE_INSTRUCTION_SET "fallback" CACHE STRING "Instruction set to use for SIMD instructions")
set_property(CACHE PE_INSTRUCTION_SET PROPERTY STRINGS fallback SSE AVX AVX2)
set(PE_MODE "release" CACHE STRING "set the build mode for Paradigm, this will influence the availability of PE_DEBUG/PE_RELEASE defines")
set_property(CACHE PE_MODE PROPERTY STRINGS release debug)
set(PE_ARCHITECTURE "AUTO" CACHE STRING "Set target architecture to build for, leave to 'AUTO' to detect and select the architecture for you.")
set_property(CACHE PE_ARCHITECTURE PROPERTY STRINGS AUTO x86 x86_64 ARM64)

OPTION(PE_USE_NATVIS "Configure project to use Visual Studio native visualizers when building for visual studio" TRUE)
OPTION(PE_BENCHMARKS "Enable the generation of benchmarks" OFF)

set(PE_BUILD_DIR "${CMAKE_BINARY_DIR}/builds/" CACHE PATH "target location where to build the project to")
set(PE_DEFINES -DUNICODE;-D_UNICODE;-DNOMINMAX CACHE INTERNAL "")
OPTION(PE_DEFAULT_COMPILE_OPTIONS "Use the default set of compile options (check 'compile options' section in the CMakelists.txt file)" TRUE)
set(ANDROID_SDK "" CACHE PATH "override root android sdk location")
set(ANDROID_BUNDLETOOL "" CACHE PATH "override android bundletool location")
set(ANDROID_GRADLE "" CACHE PATH "override gradle location")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

OPTION(PE_ECS_DISABLE_LOOKUP_CACHE_BEHAVIOUR "Disable the usage of the lookup cache for psl::ecs::state_t" OFF)

if(${PE_CCACHE})
	find_program(CCACHE "ccache")
	if(CCACHE)
		set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
		message(STATUS "found and enabled ccache")
	else()
		message(STATUS "ccache was not found")
	endif(CCACHE)
endif()

if(PE_PLATFORM STREQUAL "AUTO")
	if(WIN32)
		set(PE_PLATFORM WINDOWS)
	elseif(IOS)
		set(PE_PLATFORM IOS)
	elseif(APPLE)
		set(PE_PLATFORM MACOS)
	elseif(UNIX)
		set(PE_PLATFORM LINUX)
	elseif(ANDROID)
		set(PE_PLATFORM ANDROID)
	elseif($ENV{EMSDK} NOT STREQUAL "")
		set(PE_PLATFORM WEB)
	else()
		message(FATAL_ERROR "could not detect platform, please set `PE_PLATFORM`")
	endif()
endif()
list(APPEND PE_DEFINES -DPE_PLATFORM_${PE_PLATFORM})

if(PE_ARCHITECTURE STREQUAL "AUTO")
	if(PE_PLATFORM STREQUAL "WINDOWS" OR PE_PLATFORM STREQUAL "LINUX" OR PE_PLATFORM STREQUAL "MACOS")
		if(CMAKE_SIZEOF_VOID_P EQUAL 4)
			set(PE_ARCHITECTURE x86)
		elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(PE_ARCHITECTURE x86_64)
		else()
			message(FATAL_ERROR "we require a 32 or 64bit build platform, got sizeof void* being ${CMAKE_SIZEOF_VOID_P} bytes")
		endif()
	elseif(PE_PLATFORM STREQUAL "ANDROID")
		set(PE_ARCHITECTURE ARM64)
	elseif(PE_PLATFORM STREQUAL "WEB")
		set(PE_ARCHITECTURE WASM)
	else()
		message(FATAL_ERROR "could not detect architecture for platform ${PE_PLATFORM}")
	endif()
endif()
string(TOUPPER ${PE_ARCHITECTURE} PE_ARCHITECTURE)
list(APPEND PE_DEFINES -DPE_ARCHITECTURE_${PE_ARCHITECTURE})

if(PE_PLATFORM STREQUAL "ANDROID" AND NOT EXISTS "${CMAKE_SOURCE_DIR}/AndroidManifest.xml")
	SET(Python_ADDITIONAL_VERSIONS 3 3.0)
	find_package(PythonInterp REQUIRED)
	set(ANDROID_PY_ARGS)
	if(NOT ANDROID_SDK STREQUAL "")
		list(APPEND --sdk "${ANDROID_SDK}" ANDROID_PY_ARGS)
	endif()
	if(NOT ANDROID_GRADLE STREQUAL "")
		list(APPEND --gradle "${ANDROID_GRADLE}" ANDROID_PY_ARGS)
	endif()
	if(NOT ANDROID_BUNDLETOOL STREQUAL "")
		list(APPEND --bundletool "${ANDROID_BUNDLETOOL}" ANDROID_PY_ARGS)
	endif()
	
	string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
	add_custom_target(paradigm_android_generate ALL)
	add_custom_command(TARGET paradigm_android_generate
		COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/android.py
			--output ${PE_BUILD_DIR}/android
			${ANDROID_PY_ARGS}
		USES_TERMINAL BYPRODUCTS ${PE_BUILD_DIR}/android/gradle.properties)

	add_custom_target(paradigm_android_build ALL DEPENDS paradigm_android_generate)
	add_custom_command(TARGET paradigm_android_build
		COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/android.py
			--output ${PE_BUILD_DIR}/android
			--build gles ${PE_GLES} vulkan ${PE_VULKAN} vulkan.version ${PE_VULKAN_VERSION}
			--type ${build_type}
			${ANDROID_PY_ARGS}
		USES_TERMINAL)
	return()
endif()

if(PE_GLES)
	message("enabling GLES")
	list(APPEND PE_DEFINES -DPE_GLES)
endif()

string(TOUPPER ${PE_MODE} PE_MODE)
list(APPEND PE_DEFINES -DPE_${PE_MODE})

if(PE_VULKAN)
	message("enabling VULKAN")
	list(APPEND PE_DEFINES -DPE_VULKAN)
	if(NOT DEFINED PE_VULKAN_VERSION OR PE_VULKAN_VERSION STREQUAL "")
		message(FATAL_ERROR ${PE_VULKAN_VERSION})
		message(FATAL_ERROR "requested 'vulkan' backend, but did not set 'PE_VULKAN_VERSION'")
	endif()
endif()

if(PE_MOLTEN)
	message("transforming graphics API calls to metal using Molten")
endif()

if(PE_INSTRUCTION_SET STREQUAL "SSE")	
	list(APPEND PE_DEFINES -DINSTRUCTION_SET=1)
elseif(PE_INSTRUCTION_SET STREQUAL "AVX")	
	list(APPEND PE_DEFINES -DINSTRUCTION_SET=2)
elseif(PE_INSTRUCTION_SET STREQUAL "AVX2")	
	list(APPEND PE_DEFINES -DINSTRUCTION_SET=3)
else()
	list(APPEND PE_DEFINES -DINSTRUCTION_SET=0)
endif()

if(PE_PROFILER)
	list(APPEND PE_DEFINES -DPE_PROFILER)
endif()


if(NOT VK_STATIC)	
	list(APPEND PE_DEFINES -DVK_NO_PROTOTYPES)
elseif(VK_STATIC)
	list(APPEND PE_DEFINES -DVK_STATIC)
endif()

if(${PE_PLATFORM} STREQUAL ANDROID AND (NOT DEFINED ANDROID_NDK OR ANDROID_NDK STREQUAL ""))
		message(FATAL_ERROR "Trying to build 'android' target outside of the gradle context is not supported.")
endif()

if(PE_ECS_DISABLE_LOOKUP_CACHE_BEHAVIOUR)
	list(APPEND PE_DEFINES -DPE_ECS_DISABLE_LOOKUP_CACHE)
endif()

###############################################################################
###                    validation                                           ###
###############################################################################

if(WIN32)
	list(APPEND PE_DL_LIBS Shlwapi.lib)
endif()

###############################################################################
###                    setup output directories                             ###
###############################################################################
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
		string(TOLOWER ${OUTPUTCONFIG} OUTPUTCONFIG_FOLDERNAME)
		
		file(MAKE_DIRECTORY "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/bin")
		file(MAKE_DIRECTORY "${PE_BUILD_DIR}/${OUTPUTCONFIG_FOLDERNAME}/${ARCHI}/lib")
	endforeach()
else()		
	file(MAKE_DIRECTORY "${PE_BUILD_DIR}/default/bin")
	file(MAKE_DIRECTORY "${PE_BUILD_DIR}/default/lib")
endif()

if(PE_ANALYZE)
	if(MSVC)
	
	else()
		set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*,-readability-function-cognitive-complexity,-readability-braces-around-statements,-readability-magic-numbers,cppcoreguidelines-init-variables,cppcoreguidelines-pro-type-member-init)
	endif()
	
	#find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
	# if (CMAKE_CXX_CPPCHECK)
	# 		list(
	# 				APPEND CMAKE_CXX_CPPCHECK 
	# 						"--enable=warning"
	# 						"--inconclusive"
	# 						"--force" 
	# 						"--inline-suppr"
	# 						"--suppressions-list=${CMAKE_SOURCE_DIR}/tools/cppcheck-suppressions.txt"
	# 		)
	# endif()
	
endif()

if(PE_VERBOSE AND NOT MSVC)
	#set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
endif()
###############################################################################
###                    compile options                                      ###
###############################################################################

set(CMAKE_CXX_EXTENSIONS OFF )
set(CXX_EXTENSIONS OFF )
set(CMAKE_CXX_STANDARD 23)
list(APPEND PE_COMPILER_FEATURES cxx_std_23)

if(PE_GLES)		
		find_package(OpenGL REQUIRED)
		list(APPEND PE_DL_LIBS ${OPENGL_LIBRARIES})
endif()

set(CXX_EXTENSIONS OFF)

if(MSVC) # we currently make no distinction for CLang (todo)
	list(APPEND PE_COMPILE_OPTIONS /nologo;/MP;/sdl;/permissive-;/Zc:forScope;/Zc:inline;/Zc:rvalueCast;/Zc:wchar_t;/fp:precise;/JMC;/Zc:preprocessor)
	list(APPEND PE_COMPILE_OPTIONS /wd4505)
	if(PE_VERBOSE)
		list(APPEND PE_COMPILE_OPTIONS /W4 /w14640 /w14242 /w14254 /wd4100)	
	else()
		list(APPEND PE_COMPILE_OPTIONS /W3)
	endif()
	list(APPEND PE_DEFINES -DWIN32_LEAN_AND_MEAN)
	
	
	list(APPEND PE_COMPILE_OPTIONS_EXE
							$<$<CONFIG:Debug>:             /MDd;/Od;/RTC1;/Z7>
							$<$<CONFIG:Release>:           /MD /O2;/Oi;/Gy-;/GL;/Zi>
							$<$<CONFIG:RelWithDebInfo>:    /MD;/O2;/Oi;/Gy-;/GL;/Zo;/Z7>
							$<$<CONFIG:RelNoOptimization>: /MDd;/Od;/RTC1;/Zi>)
	list(APPEND PE_COMPILE_OPTIONS_LIB
							$<$<CONFIG:Debug>:             /MDd;/Od;/RTC1;/Z7>
							$<$<CONFIG:Release>:           /MD;/O2;/Oi;/Gy-;/Zi>
							$<$<CONFIG:RelWithDebInfo>:    /MD;/O2;/Oi;/Gy-;/Zo;/Z7>
							$<$<CONFIG:RelNoOptimization>: /MDd;/Od;/RTC1;/Zi>)
							
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/INCREMENTAL /DEBUG /OPT:NOREF /OPT:NOICF")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG")
	SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/INCREMENTAL /DEBUG /OPT:REF /OPT:NOICF /LTCG:incremental")
	SET(CMAKE_EXE_LINKER_FLAGS_RELNOOPTIMIZATION "/INCREMENTAL /OPT:NOREF /OPT:NOICF")
else()
	if(PE_VERBOSE)
		list(APPEND PE_COMPILE_OPTIONS -Wall;-Wextra)	
	endif()
	list(APPEND PE_COMPILE_OPTIONS -fstrict-aliasing;-Wno-switch;-Wno-unknown-pragmas;-Wno-unused-function;-g)	
	list(APPEND PE_COMPILE_OPTIONS
							$<$<CONFIG:Debug>:             -gdwarf-2>
							$<$<CONFIG:Release>:           -fomit-frame-pointer;-ffunction-sections;-fdata-sections>)
							
	# Add colors to ninja builds
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		add_compile_options (-fdiagnostics-color=always;-fpermissive)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	list(APPEND PE_COMPILE_OPTIONS -Wno-error=unused-command-line-argument;)
		add_compile_options (-fcolor-diagnostics)
	endif()
	
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -dead_strip -g")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -dead_strip")

	if (NOT APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fvisibility-inlines-hidden")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fvisibility-inlines-hidden")
	endif()
endif()

###############################################################################
###                    setup WSI defines                                    ###
###############################################################################

# order of decides the order of importance, this means D2D is _always_ preferred
# over any other surface type, same for all subsequent entries in the list
# note: not all surface types work on all platforms, xcb will not be selected on
# windows, while being "higher" in the list
if(PE_CORE)
set(HAS_WSI FALSE)
set(PE_SURFACE "unknown")
if(PE_PLATFORM STREQUAL "ANDROID")
	set(HAS_WSI TRUE)
	set(PE_SURFACE "ANDROID")
	list(APPEND PE_DEFINES -DSURFACE_ANDROID;-DVK_USE_PLATFORM_ANDROID_KHR)
endif()
if(NOT HAS_WSI AND PE_WSI_D2D)
	list(APPEND PE_DEFINES -DSURFACE_D2D)
	message("using D2D WSI")
	set(PE_SURFACE "D2D")
	set(HAS_WSI TRUE)
endif()
if(NOT HAS_WSI AND PE_WSI_XCB AND XCB_FOUND)
	list(APPEND PE_DEFINES -DSURFACE_XCB;-DVK_USE_PLATFORM_XCB_KHR)
	set(WSI_LIB ${XCB_LIBRARIES})
	message("using XCB WSI")
	set(PE_SURFACE "XCB")
	set(HAS_WSI TRUE)
endif()
if(NOT HAS_WSI AND PE_WSI_WIN32)
	list(APPEND PE_DEFINES -DSURFACE_WIN32;-DVK_USE_PLATFORM_WIN32_KHR)
	message("using WIN32 WSI")
	set(PE_SURFACE "WIN32")
	set(HAS_WSI TRUE)
endif()
if(NOT HAS_WSI)
	message(FATAL_ERROR "Could not find a suitable WSI, or none was selected")
endif()

string(TOLOWER ${PE_SURFACE} PE_SURFACE_LOWERCASE)
endif()
###############################################################################
###                    Add the sub projects                                 ###
###############################################################################

add_subdirectory(extern)
add_subdirectory(psl)
if(PE_CORE)
	add_subdirectory(core)
	
	if(PE_BENCHMARKS)
		add_subdirectory(benchmarks)
	endif()
endif()

if(PE_TESTS)
	add_subdirectory(tests)
endif()


if(EXISTS "${PROJECT_SOURCE_DIR}/modules.txt")
	include(${PROJECT_SOURCE_DIR}/modules.txt)
endif()
