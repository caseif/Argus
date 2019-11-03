find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)

# module name
set(MODULE_NAME "renderer")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;resman")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${SDL2_LIBRARY};${OPENGL_gl_LIBRARY};${PNG_LIBRARY}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${SDL2_INCLUDE_DIR};${OPENGL_INCLUDE_DIR};${PNG_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME})
