set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(NOT ARGUS_ROOT_DIR)
  message(FATAL_ERROR "ARGUS_ROOT_DIR must be defined")
endif()

set(PROJECT_CXX_VERSION 17)
set(PROJECT_CXX_EXTENSIONS NO)

set(LIBARGUS_NAME "argus")
set(ARGUS_INCLUDE_DIR "${CMAKE_BINARY_DIR}/dist/include")

file(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR}/tests/*.cpp)
foreach(file ${CPP_FILES})
  get_filename_component(TEST_NAME "${file}" NAME_WE)

  add_executable("${TEST_NAME}" "${file}")
  target_include_directories("${TEST_NAME}" PRIVATE "${ARGUS_INCLUDE_DIR}")
  target_link_libraries("${TEST_NAME}" PRIVATE "${LIBARGUS_NAME};Catch2::Catch2")

  # set the C++ standard
  set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD "${PROJECT_CXX_VERSION}")
  set_target_properties(${TEST_NAME} PROPERTIES CXX_EXTENSIONS "${PROJECT_CXX_EXTENSIONS}")
  set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
  # enable PIC
  set_target_properties(${TEST_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  _argus_set_compile_flags("${TEST_NAME}")
endforeach()
