find_package(OpenGL REQUIRED)

# module name
set(MODULE_NAME "render_opengl")

set(MODULE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/modules/${MODULE_NAME}")

set(MODULE_GEN_DIR "${PROJECT_BINARY_DIR}/gen/${MODULE_NAME}")
set(GEN_INC_DIR "${MODULE_GEN_DIR}/include")
set(GEN_SRC_DIR "${MODULE_GEN_DIR}/src")

# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;wm;resman;render")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${GLFW_LIBRARY};${OPENGL_LIBRARIES}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${GLFW_INCLUDE_DIR};${OPENGL_INCLUDE_DIR};${GEN_INC_DIR}")
# source paths for this module
set(LOCAL_SRC_PATHS "${GEN_SRC_DIR}")

set(EXT_TOOLING_DIR "${PROJECT_SOURCE_DIR}/external/tooling")

message("Generating OpenGL loader")

find_program(RUBY_EXE "ruby")
if(NOT RUBY_EXE)
  message(FATAL_ERROR "Ruby must be installed and available on the path")
endif()

if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
  set(BUNDLER_EXE "bundler.bat")
else()
  set(BUNDLER_EXE "bundler")
endif()

find_program(BUNDLER_PATH "${BUNDLER_EXE}")
if(NOT BUNDLER_PATH)
  message(FATAL_ERROR "Bundler must be installed and available on the path (run `gem install bundler`)")
endif()

execute_process(COMMAND "${BUNDLER_PATH}" "config" "set" "--local" "path" "./vendor/cache"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/external/tooling/aglet/"
    RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "Failed to configure Bundler: ${CMD_RES}")
endif()

execute_process(COMMAND "${BUNDLER_PATH}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/external/tooling/aglet/"
    RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "Failed to run Bundler: ${CMD_RES}")
endif()

file(MAKE_DIRECTORY "${GEN_SRC_DIR}")
execute_process(COMMAND "ruby" "${EXT_TOOLING_DIR}/aglet/aglet.rb"
    "-p" "${MODULE_SOURCE_DIR}/tooling/aglet/opengl_profile.xml"
    "-o" "${MODULE_GEN_DIR}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/external/tooling/aglet/"
    RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "    aglet.rb: ${CMD_RES}")
endif()

setup_submodule(${MODULE_NAME} 1)
