cmake_minimum_required(VERSION 3.15)
project(inproc_rt_queue_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

add_library(ring STATIC ring.cpp)
target_include_directories(ring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_ring test_ring.cpp)
target_link_libraries(test_ring PRIVATE ring)

