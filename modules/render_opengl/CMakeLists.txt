# module name
set(MODULE_NAME "render_opengl")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;resman;render")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${GLFW_LIBRARY};${OPENGL_LIBRARIES}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${GLFW_INCLUDE_DIR};${OPENGL_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME} 1)
