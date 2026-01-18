# 06-aicpu-basic example
#
# Standalone build:
#   mkdir build && cd build && cmake .. && make
#
# Or build from root project

cmake_minimum_required(VERSION 3.10)
project(06-aicpu-basic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build platform library if not already available
if(NOT TARGET ascend_platform)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../platform
                     ${CMAKE_CURRENT_BINARY_DIR}/platform_build)
endif()

add_executable(06-aicpu-basic main.cpp)
target_link_libraries(06-aicpu-basic PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(06-aicpu-basic PRIVATE -Wl,--allow-shlib-undefined)
endif()

# AICPU kernel is a separate .so - built with aarch64 cross-compiler
# See kernel/ subdirectory for kernel source
