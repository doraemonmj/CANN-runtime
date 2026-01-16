# 17-aicore-pmu example

cmake_minimum_required(VERSION 3.10)
project(17-aicore-pmu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET ascend_platform)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../platform
                     ${CMAKE_CURRENT_BINARY_DIR}/platform_build)
endif()

add_executable(17-aicore-pmu main.cpp)
target_link_libraries(17-aicore-pmu PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(17-aicore-pmu PRIVATE -Wl,--allow-shlib-undefined)
endif()
