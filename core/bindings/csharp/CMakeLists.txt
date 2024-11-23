# Make sure that the .NET SDK is installed
find_program(DOTNET_EXECUTABLE dotnet)

if(NOT DOTNET_EXECUTABLE)
    message(FATAL_ERROR "dotnet CLI not found. Please install the .NET SDK.")
endif()

# Check for the required .NET runtime version
execute_process(
    COMMAND ${DOTNET_EXECUTABLE} --list-sdks
    OUTPUT_VARIABLE DOTNET_SDKS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(PE_CSHARP_DOT_NET_VERSION "8.0")
set(PE_CSHARP_DOT_NET_RUNTIME_VERSION "8.0.11")

# Look for the required version in the output
string(REGEX MATCH "${PE_CSHARP_DOT_NET_VERSION}.[0-9]+" MATCHED_SDK ${DOTNET_SDKS})
if(MATCHED_SDK)
    message(STATUS ".NET runtime version ${PE_CSHARP_DOT_NET_VERSION} is installed. Found ${MATCHED_SDK}")
else()
    message(FATAL_ERROR ".NET runtime version ${PE_CSHARP_DOT_NET_VERSION} is not installed. Please install it. Found ${DOTNET_SDKS}")
endif()

include(src.txt)

set(PE_BINDINGS_CSHARP_PROJECT_NAME "csharp_bindings" CACHE STRING PARENT_DIRECTORY)
set(CSHARP_TARGET_FRAMEWORK "net${PE_CSHARP_DOT_NET_VERSION}")

set(CSHARP_PROJECT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(CSHARP_PROJECT_FILE "${CSHARP_PROJECT_DIR}/${PE_BINDINGS_CSHARP_PROJECT_NAME}.csproj")
set(PE_BINDINGS_CSHARP_OUTPUT_DIR "${CSHARP_PROJECT_DIR}/output" CACHE INTERNAL "")


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${PE_BINDINGS_CSHARP_PROJECT_NAME}.template
    ${CSHARP_PROJECT_FILE}
    @ONLY
)

file(MAKE_DIRECTORY ${CSHARP_PROJECT_DIR})

add_custom_target(${PE_BINDINGS_CSHARP_PROJECT_NAME}
    COMMAND ${DOTNET_EXECUTABLE} build ${PE_BINDINGS_CSHARP_PROJECT_NAME}.csproj -c ${CMAKE_BUILD_TYPE} -o ${PE_BINDINGS_CSHARP_OUTPUT_DIR}
    WORKING_DIRECTORY ${CSHARP_PROJECT_DIR}
    COMMENT "Building C# project: ${PE_BINDINGS_CSHARP_PROJECT_NAME}"
)


set(PE_BINDINGS_CSHARP_LIBRARY_PATH "${PE_BINDINGS_CSHARP_OUTPUT_DIR}/${PE_BINDINGS_CSHARP_PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}" CACHE INTERNAL "")

if(WIN32)
    set(PLATFORM_NAME "win")
elseif(UNIX)
    find_program(LSB_RELEASE_EXEC lsb_release)
    if(NOT LSB_RELEASE_EXEC)
        message(FATAL_ERROR "lsb_release not found. We need it to dynamically find the host platform's dotnet binding's host.")
    endif()
    execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
        OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(COMMAND ${LSB_RELEASE_EXEC} -rs
        OUTPUT_VARIABLE LSB_RELEASE_VERSION_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(PLATFORM_NAME "${LSB_RELEASE_ID_SHORT}.${LSB_RELEASE_VERSION_SHORT}")
elseif(APPLE)
    set(PLATFORM_NAME "osx")
else()
    message(FATAL_ERROR "Unsupported platform ${PE_PLATFORM}")
endif()

if(PE_ARCHITECTURE STREQUAL "X86")
    set(ARCHITECTURE_NAME "x86")
elseif(PE_ARCHITECTURE STREQUAL "X86_64")
    set(ARCHITECTURE_NAME "x64")
else()
    message(FATAL_ERROR "Unsupported architecture ${PE_ARCHITECTURE}")
endif()


get_filename_component(DOTNET_DIR ${DOTNET_EXECUTABLE} DIRECTORY)
if(NOT WIN32)
    # todo: this is a temp hack, we should parse the .net output.
    set(DOTNET_DIR "${DOTNET_DIR}/../lib/dotnet")
endif()
set(NETHOST_INCLUDE_DIR "${DOTNET_DIR}/packs/Microsoft.NETCore.App.Host.${PLATFORM_NAME}-${ARCHITECTURE_NAME}/${PE_CSHARP_DOT_NET_RUNTIME_VERSION}/runtimes/${PLATFORM_NAME}-${ARCHITECTURE_NAME}/native")

# Ensure these paths exist
if (NOT EXISTS "${NETHOST_INCLUDE_DIR}/nethost.h")
    message(FATAL_ERROR "nethost.h not found at ${NETHOST_INCLUDE_DIR}. Please ensure the .NET SDK is installed.")
endif()

if (NOT EXISTS "${NETHOST_INCLUDE_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}nethost${CMAKE_SHARED_LIBRARY_SUFFIX}")
    message(FATAL_ERROR "${CMAKE_SHARED_LIBRARY_PREFIX}nethost${CMAKE_SHARED_LIBRARY_SUFFIX} not found at ${NETHOST_INCLUDE_DIR}. Please ensure the .NET SDK is installed.")
endif()

set(PE_BINDINGS_CSHARP_NETHOST_INCLUDE_DIR ${NETHOST_INCLUDE_DIR} CACHE INTERNAL "")
set(PE_BINDINGS_CSHARP_NETHOST_STATIC_LIBRARY_PATH "${NETHOST_INCLUDE_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}nethost${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE INTERNAL "")

# macro to copy PE_BINDINGS_CSHARP_LIBRARY_PATH to the set TARGET's output directory
function(setup_csharp_bindings target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "Target ${target} does not exist.")
    endif()

    if(${PE_BINDINGS_CSHARP_NETHOST_INCLUDE_DIR} STREQUAL "")
        message(FATAL_ERROR "You cannot depend yet on the csharp bindings before they've been added to cmake. You will need to add your dependency after they've been initialized. This would normally put it after `core`.")
    endif()
	add_dependencies(${target} ${PE_BINDINGS_CSHARP_PROJECT_NAME})

    target_compile_definitions(${target} PUBLIC -DPE_CSHARP_BINDINGS)
    target_link_libraries(${target} PUBLIC ${PE_BINDINGS_CSHARP_NETHOST_STATIC_LIBRARY_PATH})
    target_include_directories(${target} PUBLIC ${PE_BINDINGS_CSHARP_NETHOST_INCLUDE_DIR})

    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${PE_BINDINGS_CSHARP_OUTPUT_DIR} $<TARGET_FILE_DIR:${target}>/../bin
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PE_BINDINGS_CSHARP_NETHOST_INCLUDE_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}nethost${CMAKE_SHARED_LIBRARY_SUFFIX} $<TARGET_FILE_DIR:${target}>/../bin/${CMAKE_SHARED_LIBRARY_PREFIX}nethost${CMAKE_SHARED_LIBRARY_SUFFIX}
        MAIN_DEPENDENCY ${PE_BINDINGS_CSHARP_LIBRARY_PATH}
    )
endfunction()