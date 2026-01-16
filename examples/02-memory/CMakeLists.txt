# 02-memory example

add_executable(02-memory main.cpp)
target_link_libraries(02-memory PRIVATE ascend_platform)

if(BUILD_WITH_CANN)
    target_link_options(02-memory PRIVATE -Wl,--allow-shlib-undefined)
endif()
