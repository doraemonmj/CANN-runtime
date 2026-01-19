# AICPU Kernel Build (Backend Server Pattern)
#
# Builds kernel as libscale_aicpu_kernel.so for AICPU backend server pattern
#
# Native compile on Ascend device:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.10)
project(scale_aicpu_kernel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build as shared library
add_library(scale_aicpu_kernel SHARED scale_kernel.cpp)

# Required flags for AICPU kernel
target_compile_options(scale_aicpu_kernel PRIVATE
    -fPIC           # Position independent code
    -O2             # Optimization
    -fno-exceptions # Smaller code, no exception support needed
)

# Output naming - must be libscale_aicpu_kernel.so
set_target_properties(scale_aicpu_kernel PROPERTIES
    OUTPUT_NAME "scale_aicpu_kernel"
    PREFIX "lib"    # lib prefix required
    SUFFIX ".so"    # .so extension
)
