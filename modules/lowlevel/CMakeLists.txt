# module name
set(MODULE_NAME "lowlevel")
# modules this one depends on (as a list)
set(MODULE_DEPS "")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "")

if(${USE_PTHREADS})
  list(APPEND LOCAL_LIBS "-pthread")
endif()

setup_submodule(${MODULE_NAME})
