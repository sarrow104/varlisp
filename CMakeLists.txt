cmake_minimum_required(VERSION 2.6)
add_definitions(-std=c++14)
add_definitions(-W -fexceptions -Wunused-variable -Wfatal-errors -Werror=return-type)

set(target_name "varLisp")
set(CMAKE_VERBOSE_MAKEFILE on)

set(sss "sss")
set(ss1x "ss1x")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
 add_definitions(-DNODEBUG -O2 -s)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Gprof")
 set(target_name "${target_name}G")
 set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
 add_definitions(-pg -g -O0)
else()
 set(target_name "${target_name}D")
 set(sss "sssD")
 set(ss1x "ss1xD")
 add_definitions(-O0 -g -ggdb)
endif()

project(${target_name})

find_package(Boost COMPONENTS date_time filesystem system thread iostreams)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED) # for -lssl and -lcrypto

find_library(libbrotlidec NAMES brotlidec PATHS /usr/lib/brotli NO_DEFAULT_PATH)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}")
#set(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}")
#file(GLOB_RECURSE SRC "**/*.cpp")
aux_source_directory(. SRC)
file(GLOB_RECURSE SRC2 "varlisp/*.cpp")
add_executable(${target_name} ${SRC} ${SRC2})
set(TARGET_OUTPUT_FULL_PATH ${EXECUTABLE_OUTPUT_PATH}/${target_name})
if(WIN32)
    set(TARGET_OUTPUT_FULL_PATH "${TARGET_OUTPUT_FULL_PATH}.exe")
endif(WIN32)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
 add_custom_command(
     TARGET ${target_name}
     POST_BUILD
     COMMAND ${CMAKE_STRIP} ${TARGET_OUTPUT_FULL_PATH})
endif()
#include_directories(~/extra/sss/include)
#link_directories(~/extra/sss/lib/)
target_link_libraries(${target_name} re2 ${ss1x} ${sss} uchardet gq gumbo ${Boost_LIBRARIES} rt Threads::Threads ${OPENSSL_LIBRARIES} magic z ${libbrotlidec}) # must below the bin target definition!

