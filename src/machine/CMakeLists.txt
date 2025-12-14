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

option(MACHINE_PLATFORM "Platform implementation (host|bare)" "host")
if(MACHINE_PLATFORM STREQUAL "host")
  add_compile_definitions(MACHINE_PLATFORM_HOST)
elseif(MACHINE_PLATFORM STREQUAL "bare")
  add_compile_definitions(MACHINE_PLATFORM_BARE)
else()
  message(FATAL_ERROR "Unsupported MACHINE_PLATFORM=${MACHINE_PLATFORM}")
endif()

add_library(machine SHARED
  machine.cpp
  host/platform_host.cpp
)
target_include_directories(machine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(machine PUBLIC ${EXTRA_LIBS})

