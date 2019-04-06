cmake_minimum_required(VERSION 3.13)

add_subdirectory(./../common common)
include_directories(${SHARED_LIB_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/generators)
add_executable(report_generator main.cxx)
target_link_libraries(report_generator stdc++fs)
target_link_libraries(report_generator common)
set_target_properties(report_generator PROPERTIES
    COTIRE_ADD_UNITY_BUILD FALSE
    COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/common/")
cotire(report_generator)
