# 01-device-query - Standalone Example
#
# Demonstrates querying Ascend NPU device information using ACL APIs directly.
#
# Standalone build:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.15)
project(01-device-query LANGUAGES CXX)

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
add_executable(01-device-query main.cpp)

# Include ACL headers
target_include_directories(01-device-query PRIVATE
    ${ASCEND_HOME_PATH}/include
)

# Link ACL runtime library
target_link_directories(01-device-query PRIVATE
    ${ASCEND_HOME_PATH}/lib64
)

target_link_libraries(01-device-query PRIVATE
    ascendcl
)

target_link_options(01-device-query PRIVATE
    -Wl,--allow-shlib-undefined
)
