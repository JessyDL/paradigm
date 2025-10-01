if(NOT TARGET assembler)
	message(FATAL_ERROR "PE_ASSEMBLER is required to be ON to build examples that use the assembler")
endif()

add_subdirectory(Shared)

add_subdirectory(HelloScreen)
add_subdirectory(HelloTriangle)
add_subdirectory(SimpleTextured)
add_subdirectory(PlayAudio)
add_subdirectory(Instancing)
