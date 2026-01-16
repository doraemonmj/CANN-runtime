# 02-memory - Standalone Example
#
# Demonstrates HBM memory allocation and host-device data transfers using ACL APIs.
#
# Standalone build:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.15)
project(02-memory LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# Ascend CANN runtime path
if(DEFINED ENV{ASCEND_HOME_PATH})
    set(ASCEND_HOME_PATH $ENV{ASCEND_HOME_PATH})
else()
    message(FATAL_ERROR "ASCEND_HOME_PATH is not set. CANN runtime is required.")
endif()

# Build executable
add_executable(02-memory main.cpp)

# Include ACL headers
target_include_directories(02-memory PRIVATE
    ${ASCEND_HOME_PATH}/include
)

# Link ACL runtime library
target_link_directories(02-memory PRIVATE
    ${ASCEND_HOME_PATH}/lib64
)

target_link_libraries(02-memory PRIVATE
    ascendcl
)

target_link_options(02-memory PRIVATE
    -Wl,--allow-shlib-undefined
)
