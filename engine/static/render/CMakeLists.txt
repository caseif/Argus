#find_package(GLFW REQUIRED)
#find_package(PNG REQUIRED)

# module name
set(MODULE_NAME "render")

project("${MODULE_NAME}" C CXX)

# engine modules this one depends on at runtime (as a list)
set(MODULE_ENGINE_MOD_DEPS "core;wm;resman")
# engine libraries this one depends on (as a list)
set(MODULE_ENGINE_LIB_DEPS "lowlevel")
# libraries this module depends on (as a list)
set(MODULE_LINKER_DEPS "${PNG_LIBRARY}")
# include dirs this module uses
set(MODULE_INCLUDES "${PNG_INCLUDE_DIR};${JSON_INCLUDE_DIR}")

configure_static_module(${MODULE_NAME} 0)
