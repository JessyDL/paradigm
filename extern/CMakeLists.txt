set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_CLANG_TIDY "")

include(FetchContent)

set(CACHED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.12.2
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

set(TRACY_ENABLE ${PE_ENABLE_TRACY})
FetchContent_MakeAvailable(tracy)

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

if(NOT TARGET strtype)
    add_subdirectory(strtype EXCLUDE_FROM_ALL)
endif()
set_property(TARGET strtype PROPERTY FOLDER "extern")

if(PE_TESTS)
    add_subdirectory(litmus EXCLUDE_FROM_ALL)
    set_property(TARGET litmus PROPERTY FOLDER "extern")
endif()

add_subdirectory(utfcpp EXCLUDE_FROM_ALL)
set_property(TARGET utf8cpp PROPERTY FOLDER "extern")


if(${PE_WEBGPU_NATIVE} AND NOT TARGET webgpu_dawn)
    find_package(Python3 REQUIRED)
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

    if(PE_ASSEMBLER)
        # these need to be available for the assembler to work properly
        set(TINT_BUILD_SPV_READER ON)
        set(TINT_BUILD_SPV_WRITER ON)
        set(TINT_BUILD_GLSL_WRITER ON)
        set(TINT_BUILD_WGSL_WRITER ON)
    else()
        set(TINT_BUILD_SPV_READER OFF)
    endif()

    # Disable unneeded parts        
    set(DAWN_USE_GLFW OFF)
    set(DAWN_BUILD_SAMPLES OFF)
    set(DAWN_BUILD_TESTS OFF)
    set(DAWN_BUILD_PROTOBUF OFF)

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
    set(TINT_BUILD_IR_BINARY OFF)
    set(TINT_BUILD_CMD_TOOLS OFF)
    set(TINT_ENABLE_INSTALL ON)
    FetchContent_Declare(
	    dawn
	    DOWNLOAD_COMMAND
		    cd ${FETCHCONTENT_BASE_DIR}/dawn-src &&
		    git clone --depth=1 --branch=chromium/7187 https://dawn.googlesource.com/dawn ${FETCHCONTENT_BASE_DIR}/dawn-src &&
            ${Python3_EXECUTABLE} tools/fetch_dawn_dependencies.py -s
    )
    FetchContent_MakeAvailable(dawn)
endif()

add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/dr_libs" "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio")

if(PE_ASSEMBLER)
    # assimp
    set(ASSIMP_BUILD_TESTS OFF)
    set(ASSIMP_INSTALL OFF)
    set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)
    set(ASSIMP_IGNORE_GIT_HASH ON)

    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.2
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(assimp)

    # glslang
    set(BUILD_TESTING OFF)
    set(ENABLE_SPVREMAPPER OFF)
    set(ENABLE_HLSL OFF)

    set(SKIP_GLSLANG_INSTALL ON)
    set(ENABLE_AMD_EXTENSIONS ON)
    set(ENABLE_NV_EXTENSIONS ON)
    set(ENABLE_OPT ON)
    set(ENABLE_RTTI ON)

    FetchContent_Declare(
            glslang
            DOWNLOAD_COMMAND
		        cd ${FETCHCONTENT_BASE_DIR}/glslang-src &&
		        git init &&
		        git fetch --depth=1 https://github.com/KhronosGroup/glslang.git vulkan-sdk-${PE_VULKAN_VERSION}.0 &&
		        git reset --hard FETCH_HEAD &&
                ${Python3_EXECUTABLE} ${FETCHCONTENT_BASE_DIR}/glslang-src/update_glslang_sources.py
            EXCLUDE_FROM_ALL
        )
    FetchContent_MakeAvailable(glslang)

    # spirv-cross
    set(SPIRV_CROSS_SKIP_INSTALL ON)
    FetchContent_Declare(
        SPIRV-Cross
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
        GIT_TAG vulkan-sdk-${PE_VULKAN_VERSION}.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SPIRV-Cross)

    set_target_properties(
        assimp
        UpdateAssimpLibsDebugSymbolsAndDLLs
        zlibstatic
        glslang
        GenericCodeGen
        MachineIndependent
        OSDependent
        SPIRV
        glslang-default-resource-limits
        spirv-cross
        spirv-cross-c
        spirv-cross-cpp
        spirv-cross-reflect
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-util
        spirv-cross-core
    
        PROPERTIES FOLDER "extern")
endif()

set(BUILD_SHARED_LIBS ON)
include(FetchContent)
FetchContent_Declare(
    TBB
    GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
    GIT_TAG v2022.2.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
set(TBB_TEST OFF CACHE BOOL "" FORCE)
set(TBB_EXAMPLES OFF CACHE BOOL "" FORCE)

# see https://github.com/uxlfoundation/oneTBB/issues/1274
if(PE_PLATFORM STREQUAL "ANDROID")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--undefined-version")
endif()

FetchContent_MakeAvailable(TBB)
set_target_output_directory(TARGET tbb)

set(BUILD_SHARED_LIBS ${CACHED_BUILD_SHARED_LIBS})
