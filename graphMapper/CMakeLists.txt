cmake_minimum_required(VERSION 3.13)

add_subdirectory(./../common common)
include_directories(${SHARED_LIB_INCLUDE_DIR})
add_executable(graph_mapper main.cxx)
target_link_libraries(graph_mapper stdc++fs)
target_link_libraries(graph_mapper common)
