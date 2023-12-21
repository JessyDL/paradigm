set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_CLANG_TIDY "")

add_subdirectory(fmt)
set_property(TARGET fmt PROPERTY FOLDER "extern")

option(SPDLOG_FMT_EXTERNAL "" ON)
set(spdlog_definitions "{ \"TRACE\", \"DEBUG\", \"INFO\", \"WARNING\", \"ERROR\", \"CRITICAL\", \"OFF\" }")
add_definitions(-DSPDLOG_LEVEL_NAMES=${spdlog_definitions})
add_subdirectory(spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "extern")
remove_definitions(-DSPDLOG_LEVEL_NAMES)

set(GLI_TEST_ENABLE OFF)
add_subdirectory(gli)
# silence all warnings, gli has some signed/unsigned issues and some issue /w unused variables
# todo: replace gli
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(gli INTERFACE "-w")
endif()
set_property(TARGET gli PROPERTY FOLDER "extern")
set_property(TARGET gli_dummy PROPERTY FOLDER "extern")

if(PE_TESTS)
    add_subdirectory(litmus)
    set_property(TARGET litmus PROPERTY FOLDER "extern")
endif()

add_subdirectory(utfcpp)
set_property(TARGET utf8cpp PROPERTY FOLDER "extern")

if(NOT TARGET strtype)
    add_subdirectory(strtype)
endif()
set_property(TARGET strtype PROPERTY FOLDER "extern")

if(${PE_WEBGPU_NATIVE})
    include(FetchContent)
    FetchContent_Declare(
        dawn
        GIT_REPOSITORY  https://dawn.googlesource.com/dawn
        GIT_TAG         chromium/5869
        GIT_SHALLOW     ON
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
        set(TINT_BUILD_TESTS OFF)
        set(TINT_BUILD_AS_OTHER_OS OFF)
        set(TINT_BUILD_REMOTE_COMPILE OFF)

        add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()
