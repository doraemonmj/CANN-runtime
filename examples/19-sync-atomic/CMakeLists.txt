# 19-sync-atomic example

cmake_minimum_required(VERSION 3.10)
project(19-sync-atomic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET ascend_platform)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../platform
                     ${CMAKE_CURRENT_BINARY_DIR}/platform_build)
endif()

add_executable(19-sync-atomic main.cpp)
target_link_libraries(19-sync-atomic PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(19-sync-atomic PRIVATE -Wl,--allow-shlib-undefined)
endif()
