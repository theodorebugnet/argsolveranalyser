cmake_minimum_required(VERSION 3.13)

add_subdirectory(./../common common)
include_directories(${SHARED_LIB_INCLUDE_DIR})
add_executable(benchmarker main.cxx)
target_link_libraries(benchmarker stdc++fs)
target_link_libraries(benchmarker common)
set_target_properties(benchmarker PROPERTIES
    COTIRE_ADD_UNITY_BUILD FALSE
    COTIRE_PREFIX_HEADER_IGNORE_PATH "")
cotire(benchmarker)
