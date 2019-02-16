cmake_minimum_required(VERSION 2.6)

set(SHARED_LIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(SHARED_LIB_INCLUDE_DIR ${SHARED_LIB_INCLUDE_DIR} PARENT_SCOPE)

if(TARGET common)
    message("common is already defined")
else()
    include_directories(${SHARED_LIB_INCLUDE_DIR})
    set(LIB_SRCS ./src/graph.cxx ./src/util.cxx ./src/parse.cxx ./src/SpookyV2.cpp)
    add_library(common STATIC ${LIB_SRCS})
    target_link_libraries(common Boost::program_options)
endif()
