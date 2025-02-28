set(GEN_INC_DIR "${MODULE_GENERATED_DIR}/${INCLUDE_DIR_NAME}")
set(GEN_SRC_DIR "${MODULE_GENERATED_DIR}/${SOURCE_DIR_NAME}")

set(EXT_TOOLING_DIR "${ROOT_PROJECT_DIR}/external/tooling")

message("Generating OpenGL (legacy) loader")

find_program(RUBY_EXE "ruby")
if(NOT RUBY_EXE)
  message(FATAL_ERROR "Ruby must be installed and available on the path")
endif()

if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
  find_program(BUNDLER_PATH NAMES "bundle.cmd" "bundle.bat")
else()
  find_program(BUNDLER_PATH "bundle")
endif()

if(NOT BUNDLER_PATH)
  message(FATAL_ERROR "Bundler must be installed and available on the path (run `gem install bundler`)")
endif()

execute_process(COMMAND "${BUNDLER_PATH}" "config" "set" "--local" "path" "./vendor/cache"
    WORKING_DIRECTORY "${ROOT_PROJECT_DIR}/external/tooling/aglet/"
    RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "Failed to configure Bundler: ${CMD_RES}")
endif()

execute_process(COMMAND "${BUNDLER_PATH}"
    WORKING_DIRECTORY "${ROOT_PROJECT_DIR}/external/tooling/aglet/"
    RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "Failed to run Bundler: ${CMD_RES}")
endif()

file(MAKE_DIRECTORY "${GEN_SRC_DIR}")
set(OPENGL_PROFILE_PATH "${MODULE_PROJECT_DIR}/tooling/aglet/opengl_profile.xml")
execute_process(COMMAND "ruby" "${EXT_TOOLING_DIR}/aglet/aglet.rb"
    "--lang=c"
    "-p" "${OPENGL_PROFILE_PATH}"
    "-o" "${MODULE_GENERATED_DIR}"
    WORKING_DIRECTORY "${ROOT_PROJECT_DIR}/external/tooling/aglet/"
    RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "    aglet.rb: ${CMD_RES}")
endif()
