set(LOCAL_PROJECT assembler)
include(FetchContent)

set(CMAKE_CXX_EXTENSIONS OFF )
set(CXX_EXTENSIONS OFF )
set(CMAKE_CXX_STANDARD 23)
option(AS_DEVMODE "Enable this for slightly easier dev experience where the app doesn't auto-shut by default" OFF)
option(AS_ENABLE_INSTALL "Enable installation" OFF)

# -------------------------------------------------------------------------------------------------
# Build binary
# -------------------------------------------------------------------------------------------------

include(inc.txt)
include(src.txt)
include(generators.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC})

add_executable(${LOCAL_PROJECT} ${INC} ${SRC} ${GENERATOR_INFORMATION} ${NATVIS})
set_target_output_directory(TARGET ${LOCAL_PROJECT})

if(${AS_ENABLE_INSTALL})
	install(TARGETS ${LOCAL_PROJECT} RUNTIME DESTINATION bin)
endif()

# in case paradigm isn't building the GLES backend, we still need the GL headers for KTX decoding
if(NOT PE_GLES)
    FetchContent_Declare(
        OpenGL-Registry
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenGL-Registry.git
        GIT_TAG 31db35a53f7e64d3e03e35ec47ccfd6079b792d7
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(OpenGL-Registry)
	FetchContent_Declare(
		EGL-Registry
		GIT_REPOSITORY https://github.com/KhronosGroup/EGL-Registry.git
		GIT_TAG 3ae2b7c48690d2ce13cc6db3db02dfc0572be65e
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
	)

	FetchContent_MakeAvailable(EGL-Registry)
	target_include_directories(${LOCAL_PROJECT}
		PRIVATE 
			${opengl-registry_SOURCE_DIR}/api
			${egl-registry_SOURCE_DIR}/api
	)
endif()

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "paradigm-engine/tools")
target_link_libraries(${LOCAL_PROJECT} PUBLIC
	paradigm::psl
	paradigm::core
	assimp
	spirv-cross-reflect					# reflect spirv
	spirv-cross-glsl					# spirv to gles
	SPIRV								# glsl to spirv
	glslang                             # glsl to spirv
	glslang-default-resource-limits		# default resources required to feed to glsl compiler
	$<$<BOOL:${PE_WEBGPU}>:tint_lang_wgsl_writer tint_lang_spirv_reader>
)

target_include_directories(${LOCAL_PROJECT}
	PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_definitions(${LOCAL_PROJECT}
	PUBLIC
		$<$<BOOL:${PE_WEBGPU}>:AS_ENABLE_WGSL>
	PRIVATE
		$<$<BOOL:${AS_DEVMODE}>:AS_DEVMODE>
)
