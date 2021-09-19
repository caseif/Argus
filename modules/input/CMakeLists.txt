#find_package(GLFW REQUIRED)

# module name
set(MODULE_NAME "input")
# engine modules this one depends on at runtime (as a list)
set(MODULE_ENGINE_MOD_DEPS "core;render")
# engine libraries this one depends on (as a list)
set(MODULE_ENGINE_LIB_DEPS "lowlevel")
# libraries this module depends on (as a list)
set(MODULE_LINKER_DEPS "${GLFW_LIBRARY}")
# includes this module depends on (as a list)
set(MODULE_INCLUDES "${GLFW_INCLUDE_DIR}")

configure_subproject(${MODULE_NAME} 0)
