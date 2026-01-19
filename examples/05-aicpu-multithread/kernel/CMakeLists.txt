# AICPU Kernel Build (Backend Server Pattern - Multi-threaded)
#
# Builds kernel as libparallel_add_aicpu_kernel.so for AICPU backend server pattern
# This kernel demonstrates multi-core parallel execution (8 cores)
#
# Native compile on Ascend device:
#   mkdir build && cd build && cmake .. && make

cmake_minimum_required(VERSION 3.10)
project(parallel_add_aicpu_kernel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build as shared library
add_library(parallel_add_aicpu_kernel SHARED parallel_add_kernel.cpp)

# Required flags for AICPU kernel
target_compile_options(parallel_add_aicpu_kernel PRIVATE
    -fPIC           # Position independent code
    -O2             # Optimization
    -fno-exceptions # Smaller code, no exception support needed
)

# Link pthread for multi-threading
target_link_libraries(parallel_add_aicpu_kernel PRIVATE pthread)

# Output naming - must be libparallel_add_aicpu_kernel.so
set_target_properties(parallel_add_aicpu_kernel PROPERTIES
    OUTPUT_NAME "parallel_add_aicpu_kernel"
    PREFIX "lib"    # lib prefix required
    SUFFIX ".so"    # .so extension
)
