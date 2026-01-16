# CMakeLists.txt - Ascend NPU Programming Examples
#
# This file allows building all examples together, but each example
# can also be built standalone.
#
# Build all examples:
#   mkdir build && cd build
#   cmake ..
#   make
#
# Build individual example:
#   cd examples/01-device-query
#   mkdir build && cd build
#   cmake ..
#   make

cmake_minimum_required(VERSION 3.15)
project(ascend-npu-examples VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build examples (default ON)
option(BUILD_EXAMPLES "Build all examples" ON)

# Build converted examples (01-05)
if(BUILD_EXAMPLES)
    message(STATUS "Building converted examples (01-05)")
    add_subdirectory(examples/01-device-query)
    add_subdirectory(examples/02-memory)
    add_subdirectory(examples/03-stream)
    add_subdirectory(examples/04-kernel)
    add_subdirectory(examples/05-calling-interface)

    # Note: Examples 06-20 still use platform layer (not yet converted)
    # Uncomment after conversion:
    # add_subdirectory(examples/06-aicpu-basic)
    # ... etc
endif()

message(STATUS "")
message(STATUS "===========================================")
message(STATUS "Ascend NPU Programming Examples")
message(STATUS "===========================================")
message(STATUS "Converted examples: 01-05")
message(STATUS "Remaining examples: 06-20 (deferred)")
message(STATUS "")
message(STATUS "To build individual examples:")
message(STATUS "  cd examples/XX-name/")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake .. && make")
message(STATUS "===========================================")
message(STATUS "")
