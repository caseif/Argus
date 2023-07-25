cmake_minimum_required(VERSION 3.17)

if(NOT ARGUS_ROOT_DIR)
  message(FATAL_ERROR "ARGUS_ROOT_DIR must be defined")
endif()

project(argus_tests
        LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "${ARGUS_ROOT_DIR}")

add_subdirectory("${PROJECT_SOURCE_DIR}/libs/lowlevel")

add_subdirectory("${PROJECT_SOURCE_DIR}/static/scripting")
