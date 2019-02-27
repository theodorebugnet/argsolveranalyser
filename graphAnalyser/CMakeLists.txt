cmake_minimum_required(VERSION 3.13)

add_subdirectory(./../common common)
include_directories(${SHARED_LIB_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/metrics)
add_executable(graph_analyser main.cxx)
target_link_libraries(graph_analyser stdc++fs)
target_link_libraries(graph_analyser common)
