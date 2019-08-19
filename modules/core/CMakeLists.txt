# module name
set(MODULE_NAME "core")
# modules this one depends on (as a list)
set(MODULE_DEPS "pi")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${SDL2_LIBRARY}")
# includes this module depends on (as a list)
set(LOCAL_INC_DIRS "${SDL2_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME})
