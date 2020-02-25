# module name
set(MODULE_NAME "ecs")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel;core")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "")
# includes this module depends on (as a list)
set(LOCAL_INC_DIRS "")

setup_submodule(${MODULE_NAME})
