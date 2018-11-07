cmake_minimum_required(VERSION 3.10)
set(PROJECT_PARADIGM_TEST tests CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_PARADIGM_TEST})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES CXX)


include(FetchContent)

FetchContent_Declare(
  catch
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.4.0
)

FetchContent_GetProperties(catch)
if(NOT catch_POPULATED)
  FetchContent_Populate(catch)
  add_subdirectory(${catch_SOURCE_DIR} ${catch_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


file(GLOB_RECURSE SRC src/*.cpp)
file(GLOB_RECURSE INC inc/*.h inc/*.hpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc" PREFIX "Header Files" FILES ${INC}) 
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${SRC}) 

if(VS_ADD_NATIVE_VISUALIZERS)	
	file(GLOB_RECURSE NATVIS nvs/*.natvis)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/nvs" PREFIX "natvis" FILES ${NATVIS}) 
endif()

add_executable(${LOCAL_PROJECT} ${INC} ${SRC} ${NATVIS})

set_property(TARGET ${LOCAL_PROJECT} PROPERTY FOLDER "tests")
				
target_link_libraries(${LOCAL_PROJECT} PUBLIC SHLWAPI Catch2 ${PROJECT_CORE} )



target_include_directories(${LOCAL_PROJECT}
    PUBLIC 
        $<INSTALL_INTERFACE:include>    
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(${LOCAL_PROJECT} PUBLIC ${PROJECT_COMPILER_FEATURES} PRIVATE ${PROJECT_COMPILER_FEATURES_PRIVATE})
target_compile_options(${LOCAL_PROJECT} PRIVATE ${COMPILE_OPTIONS} ${COMPILE_OPTIONS_EXE})