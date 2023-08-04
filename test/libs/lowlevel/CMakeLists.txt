cmake_minimum_required(VERSION 3.17)

if(NOT ARGUS_ROOT_DIR)
  message(FATAL_ERROR "ARGUS_ROOT_DIR must be defined")
endif()

project(test_lowlevel
    LANGUAGES CXX)

include("${ARGUS_ROOT_DIR}/cmake/test/BuildTests.cmake")

add_test(NAME "Dirtiable" COMMAND "${PROJECT_NAME}" "[Dirtiable]")
add_test(NAME "Vector" COMMAND "${PROJECT_NAME}" "[Vector]")
add_test(NAME "Matrix" COMMAND "${PROJECT_NAME}" "[Matrix]")
