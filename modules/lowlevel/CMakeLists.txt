# module name
set(MODULE_NAME "lowlevel")
# engine modules this one depends on at runtime (as a list)
set(MODULE_ENGINE_MOD_DEPS "")
# engine libraries this one depends on (as a list)
set(MODULE_ENGINE_LIB_DEPS "")
# libraries this module depends on (as a list)
if(MSVC)
  set(MODULE_LINKER_DEPS "Ws2_32")
else()
  set(MODULE_LINKER_DEPS "")
endif()

configure_subproject(${MODULE_NAME} 0)
