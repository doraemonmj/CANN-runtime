# 05-calling-interface example

add_executable(05-calling-interface main.cpp)
target_link_libraries(05-calling-interface PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(05-calling-interface PRIVATE -Wl,--allow-shlib-undefined)
endif()
