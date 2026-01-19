# 04-aicpu-kernel-launch - AICPU Kernel Launch with Backend Server Pattern
#
# Demonstrates AICPU kernel loading and execution using Runtime API
#
# Standalone build:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.15)
project(04-aicpu-kernel-launch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# Ascend CANN runtime path
if(DEFINED ENV{ASCEND_HOME_PATH})
    set(ASCEND_HOME_PATH $ENV{ASCEND_HOME_PATH})
else()
    message(FATAL_ERROR "ASCEND_HOME_PATH not set. CANN runtime required.")
endif()

# Build host executable
add_executable(04-aicpu-kernel-launch main.cpp)

# Include Runtime API headers
target_include_directories(04-aicpu-kernel-launch PRIVATE
    ${ASCEND_HOME_PATH}/include
    ${ASCEND_HOME_PATH}/include/experiment
    ${ASCEND_HOME_PATH}/include/experiment/runtime
    ${ASCEND_HOME_PATH}/include/experiment/msprof
)

# Link Runtime library
target_link_directories(04-aicpu-kernel-launch PRIVATE
    ${ASCEND_HOME_PATH}/lib64
    ${ASCEND_HOME_PATH}/runtime/lib64
)

target_link_libraries(04-aicpu-kernel-launch PRIVATE
    runtime
)

# Build AICPU kernel .so in subdirectory
add_subdirectory(kernel)
