# AICPU Kernel Build
#
# Cross-compile for aarch64 (Ascend NPU's AICPU):
#   mkdir build && cd build
#   cmake -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
#         -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ ..
#   make
#
# Native compile on Ascend device:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.10)
project(scale_kernel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build as shared library
add_library(scale_kernel SHARED scale_kernel.cpp)

# Required flags for AICPU kernel
target_compile_options(scale_kernel PRIVATE
    -fPIC           # Position independent code
    -O2             # Optimization
    -fno-exceptions # Smaller code, no exception support needed
)

# Output naming
set_target_properties(scale_kernel PROPERTIES
    OUTPUT_NAME "scale_kernel"
    PREFIX ""       # No "lib" prefix
    SUFFIX ".so"    # .so extension
)
