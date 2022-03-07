cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
add_definitions(-W -fexceptions -Wunused-variable -Wfatal-errors -Werror=return-type)
add_definitions(-DV8_COMPRESS_POINTERS)

set(target_name "varLisp")
set(CMAKE_VERBOSE_MAKEFILE on)

set(sss "sss")
set(ss1x "ss1x")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
 add_definitions(-DNODEBUG -O2)
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
find_package(restclient-cpp CONFIG REQUIRED)

#find_library(libbrotlidec NAMES brotlidec PATHS /usr/lib/brotli NO_DEFAULT_PATH)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}")
#set(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}")
#file(GLOB_RECURSE SRC "**/*.cpp")

set(MAIN "main.cpp")
set(MAIN_OMEGA "main-omegaOption.cpp")
file(GLOB_RECURSE SRC2 "src/*.cpp")
file(GLOB_RECURSE SRC_GFW "src/gfw_*.cpp")

add_executable(${target_name} ${MAIN} ${SRC2})
add_executable("test-omegaOption" ${MAIN_OMEGA} ${SRC_GFW})

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

# must below the bin target definition!
target_link_libraries(${target_name} PRIVATE restclient-cpp re2 ${ss1x} ${sss} uchardet gq gumbo ${Boost_LIBRARIES} Threads::Threads ${OPENSSL_LIBRARIES} v8 v8_libplatform magic iconv z brotlidec)
target_link_libraries("test-omegaOption" PRIVATE re2 ${ss1x} ${sss} uchardet iconv)

### tests

add_subdirectory(tests)
