# module name
set(MODULE_NAME "pi")
# modules this one depends on (as a list)
set(MODULE_DEPS "")
# libraries this module depends on (as a list)
if(MSVC)
  set(LOCAL_LIBS "Ws2_32")
else()
  set(LOCAL_LIBS "")
endif()

setup_submodule(${MODULE_NAME})
