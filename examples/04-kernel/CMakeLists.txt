# 04-kernel example

add_executable(04-kernel main.cpp)
target_link_libraries(04-kernel PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(04-kernel PRIVATE -Wl,--allow-shlib-undefined)
endif()
