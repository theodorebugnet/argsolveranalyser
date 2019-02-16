cmake_minimum_required(VERSION 3.13)

#project(graph_analyser)
#add_executable(graph_analyser main.cxx)
#include_directories(${CMAKE_SOURCE_DIR}/metrics ${CMAKE_SOURCE_DIR}/common)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.69.0 REQUIRED COMPONENTS program_options) 

if (Boost_FOUND)
    add_subdirectory(./../common common)
    include_directories(${SHARED_LIB_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/metrics)
    add_executable(graph_analyser main.cxx metrics/metric.h metrics/sizeMetric.cxx)
    target_link_libraries(graph_analyser stdc++fs)
    target_link_libraries(graph_analyser common)
endif()
