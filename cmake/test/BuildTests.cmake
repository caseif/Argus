set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(NOT ARGUS_ROOT_DIR)
  message(FATAL_ERROR "ARGUS_ROOT_DIR must be defined")
endif()

set(PROJECT_CXX_VERSION 17)
set(PROJECT_CXX_EXTENSIONS NO)

set(LIBARGUS_NAME "argus")
set(ARGUS_INCLUDE_DIR "${CMAKE_BINARY_DIR}/dist/include")

string(REPLACE "${ARGUS_ROOT_DIR}/test/" "" TEST_REL_PATH "${PROJECT_SOURCE_DIR}")

set(TEST_BINARY_DEST "${CMAKE_BINARY_DIR}/test_binaries/${TEST_REL_PATH}")

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

  _argus_set_compile_flags("${TEST_NAME}")

  add_custom_command(TARGET ${TEST_NAME} POST_BUILD
      COMMAND mkdir -p "${TEST_BINARY_DEST}"
      COMMAND "${CMAKE_COMMAND}" -E copy
      "$<TARGET_FILE:${TEST_NAME}>"
      "${TEST_BINARY_DEST}/"
      COMMENT "Copying binary for test '${TEST_NAME}' to output directory")
endforeach()
