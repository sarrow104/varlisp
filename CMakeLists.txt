cmake_minimum_required(VERSION 2.6)
add_definitions(-std=c++11)
add_definitions(-W -fexceptions -Wunused-variable -Wfatal-errors)

set(target_name "varLisp")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DNODEBUG -O2 -s)
else()
	set(target_name "${target_name}D")
	add_definitions(-O0 -g -ggdb)
endif()

project(${target_name})
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}")
#set(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}")
file(GLOB_RECURSE SRC "main.cpp" "varlisp/*.cpp" )
#aux_source_directory(. SRC)
add_executable(${target_name} ${SRC})
set(TARGET_OUTPUT_FULL_PATH ${EXECUTABLE_OUTPUT_PATH}/${target_name})
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_custom_command(
		TARGET ${target_name}
		POST_BUILD
		COMMAND ${CMAKE_STRIP} ${TARGET_OUTPUT_FULL_PATH})
endif()
#include_directories(~/extra/sss/include)
#link_directories(~/extra/sss/lib/)
target_link_libraries(${target_name} ss1x sss) # must below the bin target definition!

