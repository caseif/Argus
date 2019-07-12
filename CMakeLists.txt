set(MODULE_IDS "core;renderer")
set(MODULE_PATH_PREFIX "modules/")

cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

project(argus
        VERSION 0.1.0)

option(USE_PTHREADS "Use pthreads instead of std::thread" OFF)

if(${USE_PTHREADS})
  message("Using pthreads instead of std::thread as requested...")
endif()

function(set_module_includes)
  get_property(INCLUDE_DIRS DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
  set_property(GLOBAL PROPERTY ${PROJECT_NAME}_INCLUDE_DIRS "${INCLUDE_DIRS}")
endfunction()

function(load_dep_module_includes dest)
  foreach(module ${MODULE_DEPS})
    get_property(MODULE_INC_DIR GLOBAL PROPERTY ${module}_INCLUDE_DIRS)
    set(${dest} "${MODULE_INC_DIR} ${MODULE_INCLUDES}" PARENT_SCOPE)
  endforeach()
endfunction()

function(setup_submodule)
  project(${MODULE_NAME} C CXX)

  # source files
  file(GLOB C_FILES ${SRC_DIR}/*.c)
  file(GLOB CPP_FILES ${SRC_DIR}/*.cpp)

  # load include dirs defined by module dependencies
  load_dep_module_includes(MODULE_INCLUDES)

  include_directories(${PROJECT_SOURCE_DIR}/include
                ${MODULE_INCLUDES}
                "${CMAKE_SOURCE_DIR}/vmmlib/")

  # store this module's include dirs
  set_module_includes()
  
  if(${USE_PTHREADS})
    add_definitions("-DUSE_PTHREADS")
  endif()

  add_library(${PROJECT_NAME} OBJECT ${C_FILES} ${CPP_FILES})

  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  get_property(ALL_LIBS GLOBAL PROPERTY ALL_LIBS)
  list(APPEND ALL_LIBS "${LOCAL_LIBS}")
  set_property(GLOBAL PROPERTY ALL_LIBS "${ALL_LIBS}")

  string(REPLACE ";" " " MODULE_LIBS "${MODULE_DEPS}")
endfunction()

# transform them to add the prefix
foreach(module ${MODULE_IDS})
  list(APPEND MODULE_PATHS "${MODULE_PATH_PREFIX}${module}")
endforeach()

set(SRC_DIR src)
set(INC_DIR include)

set(CMAKE_BIN_DIR ${CMAKE_BINARY_DIR}/bin)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BIN_DIR})

foreach(module ${MODULE_IDS})
  add_subdirectory(${MODULE_PATH_PREFIX}${module})
  set(MODULE_LIBS "$<TARGET_OBJECTS:${module}>;${MODULE_LIBS}")
endforeach()

add_library(${PROJECT_NAME} SHARED ${MODULE_LIBS})
add_library(${PROJECT_NAME}_static STATIC ${MODULE_LIBS})

# set the linker language (since we're not including any sources)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME}_static PROPERTIES LINKER_LANGUAGE CXX)
# set the C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)
set_target_properties(${PROJECT_NAME}_static PROPERTIES CXX_STANDARD 11)
# enable PIC
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# add libraries for linking
get_property(ALL_LIBS GLOBAL PROPERTY ALL_LIBS)
list(REMOVE_DUPLICATES ALL_LIBS)
target_link_libraries(${PROJECT_NAME} ${ALL_LIBS})
