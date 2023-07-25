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

set(RES_INCLUDE_PATH "")
set(RES_SOURCE_FILE "")

set(PROJECT_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/test")
set(res_dir "${PROJECT_SOURCE_DIR}/res")
set(res_pack_target "pack_resources_${PROJECT_NAME}")
set(res_gen_target "gen_resources_${PROJECT_NAME}")
if(EXISTS "${res_dir}")
  set(arp_out_dir "${CMAKE_BINARY_DIR}/test/res/arp")
  set(arp_out_name "resources_${PROJECT_NAME}")
  set(arp_out_path "${arp_out_dir}/${arp_out_name}.arp")
  set(supp_mappings_path "${ARGUS_ROOT_DIR}/res/arp_custom_mappings.csv")

  file(MAKE_DIRECTORY "${arp_out_dir}")

  file(GLOB_RECURSE res_files "${res_dir}/*")
  add_custom_command(OUTPUT "${arp_out_path}"
      COMMAND "${ARPTOOL_EXE}" "pack" "${res_dir}"
      "-n" "argus"
      "-o" "${arp_out_dir}"
      "-f" "${arp_out_name}"
      "-c" "deflate"
      "-m" "${supp_mappings_path}"
      DEPENDS "${res_files}")

  add_custom_target("${res_pack_target}" DEPENDS "arptool" "${arp_out_path}")

  set(abacus_out_dir "${CMAKE_BINARY_DIR}/res/abacus")
  set(RES_INCLUDE_PATH "${PROJECT_GENERATED_DIR}/${INCLUDE_DIR_NAME}")
  set(h_out_dir "${RES_INCLUDE_PATH}")
  set(c_out_dir_base "${PROJECT_GENERATED_DIR}/${SOURCE_DIR_NAME}")
  set(c_out_dir "${c_out_dir_base}/res")

  file(MAKE_DIRECTORY "${h_out_dir}")
  file(MAKE_DIRECTORY "${c_out_dir}")

  set(arp_file_name "${arp_out_name}.arp")
  set(h_out_path "${h_out_dir}/resources.h")
  set(RES_SOURCE_FILE "${c_out_dir}/${arp_out_name}.arp.c")
  add_custom_command(OUTPUT "${RES_SOURCE_FILE}"
      COMMAND "ruby" "${CMAKE_SOURCE_DIR}/external/tooling/abacus/abacus.rb"
      "-i" "${arp_out_path}"
      "-n" "${arp_file_name}"
      "-s" "${RES_SOURCE_FILE}"
      DEPENDS "${res_pack_target}" "${arp_out_path}")

  execute_process(COMMAND "ruby" "${CMAKE_SOURCE_DIR}/external/tooling/abacus/abacus.rb"
      "-n" "${arp_out_name}.arp"
      "-h" "${h_out_path}"
      RESULT_VARIABLE CMD_RES)
  if(CMD_RES)
    message(FATAL_ERROR "    abacus.rb: ${CMD_RES}")
  endif()

  add_custom_target("${res_gen_target}" DEPENDS "${res_pack_target}" "${h_out_path}" "${RES_SOURCE_FILE}")
endif()

file(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR}/tests/*.cpp)
foreach(file ${CPP_FILES})
  get_filename_component(TEST_NAME "${file}" NAME_WE)

  add_executable("${TEST_NAME}" "${file};${RES_SOURCE_FILE}")
  target_include_directories("${TEST_NAME}" PRIVATE "${ARGUS_INCLUDE_DIR};${RES_INCLUDE_PATH};"
      "${PROJECT_SOURCE_DIR}/include;${ARGUS_ROOT_DIR}/test/include")
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
