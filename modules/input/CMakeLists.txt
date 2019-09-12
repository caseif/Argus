# module name
set(MODULE_NAME "input")
# modules this one depends on (as a list)
set(MODULE_DEPS "core")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "SDL2")

setup_submodule(${MODULE_NAME})
