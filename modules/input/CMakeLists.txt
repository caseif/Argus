find_package(GLFW REQUIRED)
find_package(SDL2 REQUIRED)

# module name
set(MODULE_NAME "input")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;renderer")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${GLFW_LIBRARY};${SDL2_LIBRARY}")
# includes this module depends on (as a list)
set(LOCAL_INC_DIRS "${GLFW_INCLUDE_DIR};${SDL2_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME})
