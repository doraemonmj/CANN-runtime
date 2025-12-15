cmake_minimum_required(VERSION 3.15)
project(inproc_queue_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(inproc_queue main.cpp)

