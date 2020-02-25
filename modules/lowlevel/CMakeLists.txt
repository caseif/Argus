# module name
set(MODULE_NAME "lowlevel")
# modules this one depends on (as a list)
set(MODULE_DEPS "")
# libraries this module depends on (as a list)
if(MSVC)
  set(LOCAL_LIBS_SHARED "Ws2_32")
  set(LOCAL_LIBS_STATIC "Ws2_32")
else()
  set(LOCAL_LIBS_SHARED "")
  set(LOCAL_LIBS_STATIC "")
endif()

setup_submodule(${MODULE_NAME})
