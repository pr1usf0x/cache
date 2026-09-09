cmake_minimum_required(VERSION 4.4)

project(cache)

# deps
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

option(TEST "Run test programs" OFF)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL
    "Generate compile_commands.json" FORCE)

set(TARGET cache)

add_executable(${TARGET})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories("include")

#tests 
include(GoogleTest)

add_executable(cache_algorithm_test ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/cache_algorithm.cpp)
target_link_libraries(cache_algorithm_test PRIVATE GTest::gtest_main)

gtest_discover_tests(cache_algorithm_test)