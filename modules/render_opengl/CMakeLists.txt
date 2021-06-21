find_package(OpenGL REQUIRED)

# module name
set(MODULE_NAME "render_opengl")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;wm;resman;render")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${GLFW_LIBRARY};${OPENGL_LIBRARIES}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${GLFW_INCLUDE_DIR};${OPENGL_INCLUDE_DIR}")
# source paths for this module
set(LOCAL_SRC_PATHS "${PROJECT_BINARY_DIR}/src.tmp")

set(TOOLING_DIR "${PROJECT_SOURCE_DIR}/tooling")

set(TEMP_SRC_DIR "${PROJECT_BINARY_DIR}/modules/${MODULE_NAME}/src.tmp")

message("Generating glext.cpp")
file(MAKE_DIRECTORY "${TEMP_SRC_DIR}")
execute_process(COMMAND "ruby" "${TOOLING_DIR}/gl/gen_glext_src.rb"
    "${TEMP_SRC_DIR}/glext.cpp"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RESULT_VARIABLE CMD_RES)
if(CMD_RES)
message(FATAL_ERROR "    gen_glext_src.rb: ${CMD_RES}")
endif()

setup_submodule(${MODULE_NAME} 1)
