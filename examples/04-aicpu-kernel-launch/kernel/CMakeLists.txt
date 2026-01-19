# AICPU Kernel Build (Backend Server Pattern)
#
# Builds kernel as libadd_aicpu_kernel.so for AICPU backend server pattern
#
# Native compile on Ascend device:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.10)
project(add_aicpu_kernel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build as shared library
add_library(add_aicpu_kernel SHARED add_kernel.cpp)

# Required flags for AICPU kernel
target_compile_options(add_aicpu_kernel PRIVATE
    -fPIC           # Position independent code
    -O2             # Optimization
    -fno-exceptions # Smaller code, no exception support needed
)

# Output naming - must be libadd_aicpu_kernel.so
set_target_properties(add_aicpu_kernel PROPERTIES
    OUTPUT_NAME "add_aicpu_kernel"
    PREFIX "lib"    # lib prefix required
    SUFFIX ".so"    # .so extension
)
