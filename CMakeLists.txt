# CMakeLists.txt - Ascend NPU Platform Library
#
# Build with:
#   mkdir build && cd build
#   cmake .. -DBUILD_WITH_CANN=ON
#   make
#
# On host without CANN:
#   cmake .. -DBUILD_WITH_CANN=OFF
#   make

cmake_minimum_required(VERSION 3.10)
project(ascend-platform VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_WITH_CANN "Build with CANN runtime support" OFF)
option(BUILD_EXAMPLES "Build examples" ON)

# CANN paths
set(CANN_PATH "/usr/local/Ascend/ascend-toolkit/latest" CACHE PATH "CANN toolkit path")

# Platform library (named ascend_platform to avoid conflict with CANN's libplatform.so)
add_library(ascend_platform SHARED
    platform/src/platform.cpp
)

target_include_directories(ascend_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/include
)

if(BUILD_WITH_CANN)
    # Detect platform (x86_64 or aarch64)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(CANN_ARCH "aarch64-linux")
    else()
        set(CANN_ARCH "x86_64-linux")
    endif()

    set(CANN_INCLUDE "${CANN_PATH}/${CANN_ARCH}/include")
    set(CANN_LIB "${CANN_PATH}/${CANN_ARCH}/lib64")

    message(STATUS "Building with CANN support")
    message(STATUS "  CANN_PATH: ${CANN_PATH}")
    message(STATUS "  CANN_INCLUDE: ${CANN_INCLUDE}")
    message(STATUS "  CANN_LIB: ${CANN_LIB}")

    target_compile_definitions(ascend_platform PRIVATE BUILD_WITH_CANN)
    target_include_directories(ascend_platform PRIVATE
        ${CANN_INCLUDE}
        ${CANN_INCLUDE}/experiment/runtime
        ${CANN_INCLUDE}/experiment/msprof
    )
    target_link_directories(ascend_platform PRIVATE
        ${CANN_LIB}
        ${CANN_LIB}/stub
    )
    target_link_libraries(ascend_platform PRIVATE
        ascendcl
        runtime
    )
    # Set RPATH for runtime library loading
    set_target_properties(ascend_platform PROPERTIES
        INSTALL_RPATH "${CANN_LIB}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
else()
    message(STATUS "Building without CANN (simulation mode)")
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples/01-device-query)
    add_subdirectory(examples/02-memory)
    add_subdirectory(examples/03-stream)
endif()
