cmake_minimum_required(VERSION 3.15)
project(addworker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(addworker SHARED add.cpp)
target_include_directories(addworker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

