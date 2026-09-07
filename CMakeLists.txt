cmake_minimum_required(VERSION 4.4)

project(cache)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL
    "Generate compile_commands.json" FORCE)

set(TARGET cache)

add_executable(${TARGET})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

include_directories("src/include")
