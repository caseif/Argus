message("Configuring arptool")
set(arptool_wd "${CMAKE_BINARY_DIR}/tooling/arptool")

set(arptool_exe_dir "${CMAKE_BINARY_DIR}/tooling/arptool")

execute_process(COMMAND "${CMAKE_COMMAND}" -E make_directory "${arptool_wd}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-G" "${CMAKE_GENERATOR}"
                        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                        "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
                        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${arptool_exe_dir}"
                        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${arptool_exe_dir}"
                        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${arptool_exe_dir}"
                        "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
                        "${CMAKE_SOURCE_DIR}/external/tooling/arptool"
                WORKING_DIRECTORY "${arptool_wd}"
                RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "    Failed to configure arptool: ${CMD_RES}")
endif()

set(arptool_exe "${arptool_exe_dir}/arptool${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_command(OUTPUT "${arptool_exe}"
                  COMMAND "${CMAKE_COMMAND}" "--build" "."
                  DEPENDS "${arptool_source_files}"
                  WORKING_DIRECTORY "${arptool_wd}")
add_custom_target("arptool" DEPENDS "${arptool_exe}")
