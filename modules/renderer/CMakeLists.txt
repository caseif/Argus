# module name
set(MODULE_NAME "renderer")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${SDL2_LIBRARY};${OPENGL_gl_LIBRARY}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${VMMLIB_INCLUDE_DIR};${SDL2_INCLUDE_DIR};${OPENGL_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME})
