# todo: support ctest with litmus
# include(CTest)
include(inc.txt)
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(PE_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

add_executable(tests ${INC} ${SRC} ${NATVIS})
add_executable(paradigm::tests ALIAS tests)

set_property(TARGET tests PROPERTY FOLDER "tests")
target_link_libraries(tests PUBLIC ${SHLWAPI} paradigm::psl litmus)
set_target_output_directory(tests)
set_target_properties(tests PROPERTIES LINKER_LANGUAGE CXX)

if(PE_PLATFORM STREQUAL "WEB")
    set_target_properties(tests PROPERTIES SUFFIX ".html")
endif()

target_include_directories(tests
    PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(tests PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(tests PRIVATE ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_EXE})

if (MSVC)
	target_compile_options(tests PRIVATE /bigobj)
endif()
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(tests PRIVATE -Wno-unused-comparison)
endif()
