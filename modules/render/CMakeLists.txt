#find_package(GLFW REQUIRED)
#find_package(PNG REQUIRED)

# module name
set(MODULE_NAME "render")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;ecs;resman")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${GLFW_LIBRARY};${OPENGL_LIBRARIES};${PNG_LIBRARY}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${GLFW_INCLUDE_DIR};${OPENGL_INCLUDE_DIR};${PNG_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME})
