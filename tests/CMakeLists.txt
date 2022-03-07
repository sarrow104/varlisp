cmake_minimum_required(VERSION 3.20)

project(unit-test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE $ENV{CMAKE_TOOLCHAIN_FILE})
message(VERBOSE ${CMAKE_TOOLCHAIN_FILE})

message(VERBOSE ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH "~/vcpkg/packages/gtest_x64-osx/share/GTest/")
message(VERBOSE ${CMAKE_PREFIX_PATH})

add_definitions(-DV8_COMPRESS_POINTERS)
add_definitions(-W -fexceptions -Wunused-variable -Wfatal-errors -Werror=return-type)

#add_link_options(-L /Users/sarrow/vcpkg/installed/x64-osx/lib)

enable_testing()

#https://github.com/google/googletest/issues/702
find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(unit-test xxx_tests.cpp) # add this executable

# https://stackoverflow.com/questions/41315739/vcpkg-does-not-work-for-google-test
target_link_libraries(unit-test PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main fmt::fmt-header-only)

#include(GoogleTest)
#gtest_discover_tests(mytests
#  PROPERTIES
#    LABELS "unit"
#  DISCOVERY_TIMEOUT  # how long to wait (in seconds) before crashing
#    240
#  )

add_test(NAME varlisp-gtest COMMAND unit-test)

######
add_executable(unit-test-v8env v8env_tests.cpp ../src/detail/v8env.cpp)
target_link_libraries(unit-test-v8env PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main fmt::fmt-header-only v8 v8_libplatform sss iconv)
add_test(NAME varlisp-gtest-v8env COMMAND unit-test-v8env)
