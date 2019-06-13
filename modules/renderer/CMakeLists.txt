# module name
set(MODULE_NAME "renderer")
# modules this one depends on (as a list)
set(MODULE_DEPS "core")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "")

setup_submodule(${MODULE_NAME})
