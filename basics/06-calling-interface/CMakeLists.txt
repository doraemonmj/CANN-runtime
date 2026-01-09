cmake_minimum_required(VERSION 3.10)
project(calling_interface)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directory to include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Build add_kernel as shared library
add_library(add_kernel SHARED
    kernel/add_kernel.cpp
)

# Build mul_kernel as shared library
add_library(mul_kernel SHARED
    kernel/mul_kernel.cpp
)

# Build main executable
add_executable(main
    main.cpp
)

# Link dl library for dlopen/dlsym
# On macOS, dl functions are in libSystem (linked by default)
# On Linux, need to explicitly link dl
if(UNIX AND NOT APPLE)
    target_link_libraries(main dl)
endif()

# Set output directory for shared libraries to be in the same directory as executable
set_target_properties(add_kernel PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(mul_kernel PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
