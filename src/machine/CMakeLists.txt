cmake_minimum_required(VERSION 3.15)
project(machine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)
set(EXTRA_LIBS Threads::Threads)
if(UNIX AND NOT APPLE)
  list(APPEND EXTRA_LIBS dl)
endif()

add_library(machine SHARED machine.cpp)
target_include_directories(machine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(machine PUBLIC ${EXTRA_LIBS})

