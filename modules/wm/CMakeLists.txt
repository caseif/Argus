#find_package(GLFW REQUIRED)
#find_package(PNG REQUIRED)

# module name
set(MODULE_NAME "wm")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${GLFW_LIBRARY}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${GLFW_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME} 0)
