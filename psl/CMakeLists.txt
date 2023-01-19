include(inc.txt) 
include(src.txt)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc/psl" PREFIX "inc" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SRC}) 

if(PE_USE_NATVIS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

find_package (Threads)

add_library(psl STATIC ${INC} ${SRC} ${NATVIS})
add_library(paradigm::psl ALIAS psl)

if(${PE_PCH})
	target_precompile_headers(psl PUBLIC ${INC_PCH})
endif()

target_include_directories(psl
    PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)
target_compile_definitions(psl PUBLIC "SPDLOG_FMT_EXTERNAL" ${PE_DEFINES})

target_link_libraries(psl PUBLIC fmt::fmt ${CMAKE_THREAD_LIBS_INIT} spdlog utf8cpp strtype)
set_target_output_directory(psl)
set_target_properties(psl PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET psl PROPERTY FOLDER "paradigm-engine")
target_compile_features(psl PUBLIC ${PE_COMPILER_FEATURES})
target_compile_options(psl PUBLIC ${PE_COMPILE_OPTIONS} ${PE_COMPILE_OPTIONS_LIB})

if(PE_DEV_MAKE_PSL_EXE)
	add_subdirectory(main)
endif()
