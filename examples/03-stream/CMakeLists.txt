# 03-stream example

add_executable(03-stream main.cpp)
target_link_libraries(03-stream PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(03-stream PRIVATE -Wl,--allow-shlib-undefined)
endif()
