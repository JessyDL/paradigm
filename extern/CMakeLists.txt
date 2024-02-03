set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_CLANG_TIDY "")

add_subdirectory(fmt EXCLUDE_FROM_ALL)
set_property(TARGET fmt PROPERTY FOLDER "extern")

option(SPDLOG_FMT_EXTERNAL "" ON)
set(spdlog_definitions "{ \"TRACE\", \"DEBUG\", \"INFO\", \"WARNING\", \"ERROR\", \"CRITICAL\", \"OFF\" }")
add_definitions(-DSPDLOG_LEVEL_NAMES=${spdlog_definitions})
add_subdirectory(spdlog EXCLUDE_FROM_ALL)
set_property(TARGET spdlog PROPERTY FOLDER "extern")
remove_definitions(-DSPDLOG_LEVEL_NAMES)

set(GLI_TEST_ENABLE OFF)
add_subdirectory(gli EXCLUDE_FROM_ALL)
# silence all warnings, gli has some signed/unsigned issues and some issue /w unused variables
# todo: replace gli
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(gli INTERFACE "-w")
endif()
set_property(TARGET gli PROPERTY FOLDER "extern")
set_property(TARGET gli_dummy PROPERTY FOLDER "extern")

if(PE_TESTS)
    add_subdirectory(litmus EXCLUDE_FROM_ALL)
    set_property(TARGET litmus PROPERTY FOLDER "extern")
endif()

add_subdirectory(utfcpp EXCLUDE_FROM_ALL)
set_property(TARGET utf8cpp PROPERTY FOLDER "extern")

if(NOT TARGET strtype)
    add_subdirectory(strtype EXCLUDE_FROM_ALL)
endif()
set_property(TARGET strtype PROPERTY FOLDER "extern")

if(${PE_WEBGPU_NATIVE})
    # some of the understandings in making this work are from https://github.com/eliemichel/webgpu-native
    # after chromium/5931 dawn started having issues with the cmake build and with build times.
    include(FetchContent)

    FetchContent_Declare(
	dawn
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/dawn-src &&
		git init &&
		git fetch --depth=1 https://dawn.googlesource.com/dawn chromium/6278 &&
		git reset --hard FETCH_HEAD
    )

    FetchContent_GetProperties(dawn)
    if (NOT dawn_POPULATED)
        FetchContent_Populate(dawn)
        set(DAWN_FETCH_DEPENDENCIES ON) # use manual method so we don't want to pull in depot_tools

        if (APPLE)
            set(USE_VULKAN OFF)
            set(USE_METAL ON)
        else()
            set(USE_VULKAN ON)
            set(USE_METAL OFF)
        endif()
        set(DAWN_ENABLE_D3D11 OFF)
        set(DAWN_ENABLE_D3D12 OFF)
        set(DAWN_ENABLE_METAL ${USE_METAL})
        set(DAWN_ENABLE_NULL OFF)
        set(DAWN_ENABLE_DESKTOP_GL OFF)
        set(DAWN_ENABLE_OPENGLES OFF)
        set(DAWN_ENABLE_VULKAN ${USE_VULKAN})
        set(TINT_BUILD_SPV_READER OFF)

        # Disable unneeded parts
        
        set(DAWN_USE_GLFW OFF)
        set(DAWN_BUILD_SAMPLES OFF)
        set(TINT_BUILD_TINT OFF)
        set(TINT_BUILD_SAMPLES OFF)
        set(TINT_BUILD_DOCS OFF)
        set(TINT_BUILD_TESTS OFF)
        set(TINT_BUILD_FUZZERS OFF)
        set(TINT_BUILD_SPIRV_TOOLS_FUZZER OFF)
        set(TINT_BUILD_AST_FUZZER OFF)
        set(TINT_BUILD_REGEX_FUZZER OFF)
        set(TINT_BUILD_BENCHMARKS OFF)
        set(TINT_BUILD_AS_OTHER_OS OFF)
        set(TINT_BUILD_REMOTE_COMPILE OFF)
        set(TINT_BUILD_CMD_TOOLS OFF)

        add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    target_include_directories(dawn_utils PUBLIC "${CMAKE_BINARY_DIR}/_deps/dawn-src/src")
endif()
