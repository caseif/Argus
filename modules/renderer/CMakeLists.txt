# module name
set(MODULE_NAME "renderer")
# modules this one depends on (as a list)
set(MODULE_DEPS "core")
# libraries this module depends on (as a list)
set(LOCAL_LIBS "SDL2;GL")
# include dirs this module uses
set(LOCAL_INC_DIRS "${CMAKE_SOURCE_DIR}/vmmlib/")

setup_submodule(${MODULE_NAME})
