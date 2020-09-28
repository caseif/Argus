# module name
set(MODULE_NAME "core")
# modules this one depends on (as a list)
set(MODULE_DEPS "lowlevel")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "")
if(UNIX)
  list(APPEND LOCAL_LIBS "dl")
endif()
# includes this module depends on (as a list)
set(LOCAL_INC_DIRS "")

setup_submodule(${MODULE_NAME} 0)
