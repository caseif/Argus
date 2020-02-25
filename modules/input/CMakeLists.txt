#find_package(GLFW REQUIRED)

# module name
set(MODULE_NAME "input")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;render")
# libraries this module depends on (as a list)
set(LOCAL_LIBS_SHARED "${GLFW_LIBRARY_SHARED}")
set(LOCAL_LIBS_STATIC "${GLFW_LIBRARY_STATIC}")
# includes this module depends on (as a list)
set(LOCAL_INC_DIRS "${GLFW_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME})
