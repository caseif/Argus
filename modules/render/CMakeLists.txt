#find_package(GLFW REQUIRED)
#find_package(PNG REQUIRED)

# module name
set(MODULE_NAME "render")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core;wm;resman")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${PNG_LIBRARY}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${PNG_INCLUDE_DIR};${JSON_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME} 0)
