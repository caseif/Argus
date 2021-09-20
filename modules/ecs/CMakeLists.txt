# module name
set(MODULE_NAME "ecs")
# engine modules this one depends on at runtime (as a list)
set(MODULE_ENGINE_MOD_DEPS "core")
# engine libraries this one depends on (as a list)
set(MODULE_ENGINE_LIB_DEPS "lowlevel")
# libraries this module depends on (as a list)
set(MODULE_LINKER_DEPS "")
# includes this module depends on (as a list)
set(MODULE_INCLUDES "")

configure_static_module(${MODULE_NAME} 0)
