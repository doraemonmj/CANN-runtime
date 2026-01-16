# 13-cube-memory example

cmake_minimum_required(VERSION 3.10)
project(13-cube-memory LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET ascend_platform)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../platform
                     ${CMAKE_CURRENT_BINARY_DIR}/platform_build)
endif()

add_executable(13-cube-memory main.cpp)
target_link_libraries(13-cube-memory PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(13-cube-memory PRIVATE -Wl,--allow-shlib-undefined)
endif()
