# 05-aicpu-multithread - AICPU Multi-core Parallel Execution
#
# Demonstrates parallel execution across multiple AICPU cores
#
# Standalone build:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.15)
project(05-aicpu-multithread LANGUAGES CXX)

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
add_executable(05-aicpu-multithread main.cpp)

# Include Runtime API headers
target_include_directories(05-aicpu-multithread PRIVATE
    ${ASCEND_HOME_PATH}/include
    ${ASCEND_HOME_PATH}/include/experiment
    ${ASCEND_HOME_PATH}/include/experiment/runtime
    ${ASCEND_HOME_PATH}/include/experiment/msprof
)

# Link Runtime library
target_link_directories(05-aicpu-multithread PRIVATE
    ${ASCEND_HOME_PATH}/lib64
    ${ASCEND_HOME_PATH}/runtime/lib64
)

target_link_libraries(05-aicpu-multithread PRIVATE
    runtime
)

# Build AICPU kernel .so in subdirectory
add_subdirectory(kernel)
