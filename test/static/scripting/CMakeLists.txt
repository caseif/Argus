cmake_minimum_required(VERSION 3.17)

if(NOT ARGUS_ROOT_DIR)
  message(FATAL_ERROR "ARGUS_ROOT_DIR must be defined")
endif()

project(test_scripting
    LANGUAGES CXX)

set(PROJECT_CXX_VERSION 17)
set(PROJECT_CXX_EXTENSIONS NO)

include("${ARGUS_ROOT_DIR}/cmake/test/BuildTests.cmake")
