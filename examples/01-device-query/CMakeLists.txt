# 01-device-query example

add_executable(01-device-query main.cpp)
target_link_libraries(01-device-query PRIVATE ascend_platform)

# Allow undefined symbols to be resolved at runtime
if(BUILD_WITH_CANN)
    target_link_options(01-device-query PRIVATE -Wl,--allow-shlib-undefined)
endif()
