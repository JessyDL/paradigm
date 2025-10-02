if(NOT TARGET assembler)
	message(FATAL_ERROR "PE_ASSEMBLER is required to be ON to build examples that use the assembler")
endif()

add_subdirectory(Shared)

add_example(HelloScreen)
add_example(HelloTriangle)
add_example(SimpleTextured)
add_example(PlayAudio)
add_example(Instancing)
