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

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "tools")
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
