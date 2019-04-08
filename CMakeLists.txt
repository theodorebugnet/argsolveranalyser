cmake_minimum_required(VERSION 3.13)
project(solveranalyser LANGUAGES CXX)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(cotire)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.69.0 REQUIRED COMPONENTS program_options) 

SET(CMAKE_CXX_FLAGS "-Wall -Wextra")
SET(CMAKE_CXX_FLAGS_DEBUG "-g")# -pg")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")

#Customise these values to customise the installation
SET(INSTALL_PREF "../../install-test/") #Unset for installation to default binary directory, or change to a different portable directory
SET(STORE_PATH "./store/") #Unless installing to a self-contained location, this should probably be an absolute path, e.g. inside ~
SET(CONF_PREFIX "./conf/") #Same for this, alternatively set this to a path in /etc/
SET(CONF_PATH_ANALYSER "./graph_analyser.conf") #The next three are names of files inside CONF_PREFIX
SET(CONF_PATH_MAPPER "./graph_mapper.conf")
SET(CONF_PATH_BENCHMARKER "./benchmarker.conf")
SET(CONF_PATH_GENERATOR "./report_generator.conf")

cmake_policy(SET CMP0069 NEW)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
set(COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES "1" CACHE STRING "" FORCE)

if (Boost_FOUND)
    set(CMAKE_CXX_STANDARD 17)
    set(CXX_STANDARD_REQUIRED ON)
    add_subdirectory(graphAnalyser)
    add_subdirectory(graphMapper)
    add_subdirectory(benchmarker)
    add_subdirectory(reportGenerator)
    target_compile_definitions(graph_analyser PRIVATE CONF_PATH="${CONF_PREFIX}${CONF_PATH_ANALYSER}")
    target_compile_definitions(graph_mapper PRIVATE CONF_PATH="${CONF_PREFIX}${CONF_PATH_MAPPER}")
    target_compile_definitions(report_generator PRIVATE CONF_PATH="${CONF_PREFIX}${CONF_PATH_GENERATOR}" STORE_PATH="${STORE_PATH}")
    target_compile_definitions(benchmarker PRIVATE CONF_PATH="${CONF_PREFIX}${CONF_PATH_BENCHMARKER}" BIN_PATH="${INSTALL_PREF}")
    target_compile_definitions(common PRIVATE STORE_PATH="${STORE_PATH}")
    set(CMAKE_INSTALL_PREFIX ${INSTALL_PREF} CACHE PATH "..." FORCE)
    install(TARGETS graph_analyser RUNTIME DESTINATION .)
    install(TARGETS graph_mapper RUNTIME DESTINATION .)
    install(TARGETS benchmarker RUNTIME DESTINATION .)
    install(TARGETS report_generator RUNTIME DESTINATION .)
    install(DIRECTORY store DESTINATION . USE_SOURCE_PERMISSIONS MESSAGE_NEVER)
    install(PROGRAMS benchmarker/external/exts/compare-extensions DESTINATION ${INSTALL_PREF})
    install(PROGRAMS benchmarker/external/runsolver/runsolver DESTINATION ${INSTALL_PREF})
endif()
