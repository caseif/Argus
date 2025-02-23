set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(NOT ARGUS_ROOT_DIR)
  message(FATAL_ERROR "ARGUS_ROOT_DIR must be defined")
endif()

set(PROJECT_CXX_VERSION 17)
set(PROJECT_CXX_EXTENSIONS NO)

set(ARGUS_LIBRARY "argus")
set(ARGUS_DIST_DIR "${CMAKE_BINARY_DIR}/dist")
set(ARGUS_INCLUDE_DIR "${CMAKE_BINARY_DIR}/dist/include")
set(ARGUS_LIB_DIR "${CMAKE_BINARY_DIR}/dist/lib")

string(REPLACE "${ARGUS_ROOT_DIR}/test/" "" TEST_REL_PATH "${PROJECT_SOURCE_DIR}")

set(TEST_BINARY_DEST "${CMAKE_BINARY_DIR}/test_binaries/${TEST_REL_PATH}")

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/tests/*.cpp)

add_executable("${PROJECT_NAME}" "${SRC_FILES}")
target_include_directories("${PROJECT_NAME}" PRIVATE "${ARGUS_INCLUDE_DIR};${RES_INCLUDE_PATH};"
    "${PROJECT_SOURCE_DIR}/include;${ARGUS_ROOT_DIR}/test/include")
target_link_libraries("${PROJECT_NAME}" PRIVATE "${ARGUS_LIBRARY};Catch2::Catch2WithMain")

# set the C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD "${PROJECT_CXX_VERSION}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS "${PROJECT_CXX_EXTENSIONS}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)

_argus_set_compile_flags("${PROJECT_NAME}")
target_compile_options("${PROJECT_NAME}" PRIVATE "$<$<CXX_COMPILER_ID:GNU>:-Wno-unused-but-set-variable>")
if((CLANG AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 10) OR
  (GCC AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 12))
  target_compile_options("${PROJECT_NAME}" PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:-Wno-c++20-extensions>")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${TEST_BINARY_DEST}"
    COMMAND "${CMAKE_COMMAND}" -E copy
    "$<TARGET_FILE:${PROJECT_NAME}>"
    "${TEST_BINARY_DEST}/"
    COMMENT "Copying binary for test set '${PROJECT_NAME}' to output directory")

if(ENABLE_CTEST)
  enable_testing()
endif()

list(APPEND CMAKE_MODULE_PATH "${CATCH2_SOURCE_DIR}/extras")
include(CTest)
include(Catch)
catch_discover_tests("${PROJECT_NAME}"
    WORKING_DIRECTORY "${ARGUS_LIB_DIR}")
