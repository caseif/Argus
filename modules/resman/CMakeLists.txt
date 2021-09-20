# module name
set(MODULE_NAME "resman")
# engine modules this one depends on at runtime (as a list)
set(MODULE_ENGINE_MOD_DEPS "core")
# engine libraries this one depends on (as a list)
set(MODULE_ENGINE_LIB_DEPS "lowlevel")
# libraries this module depends on (as a list)
set(MODULE_LINKER_DEPS "${ARP_LIBRARY}")
# include dirs this module uses
set(MODULE_INCLUDES "${ARP_INCLUDE_DIR}")

configure_static_module(${MODULE_NAME} 0)
