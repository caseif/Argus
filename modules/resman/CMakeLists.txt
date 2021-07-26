# module name
set(MODULE_NAME "resman")
# modules this one depends on (as a list)
set(MODULE_DEPS "core;lowlevel")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "${ARP_LIBRARY}")
# include dirs this module uses
set(LOCAL_INC_DIRS "${ARP_INCLUDE_DIR}")

setup_submodule(${MODULE_NAME} 0)
