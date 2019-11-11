set(MODULE_IDS "lowlevel;core;input;resman;renderer")
set(MODULE_PATH_PREFIX "modules/")

cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

project(argus
        VERSION 0.1.0)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(USE_PTHREADS "Use pthreads instead of std::thread" OFF)
option(USE_GLES "Use ES profile for OpenGL" OFF)

if(${USE_PTHREADS})
  message("Using pthreads instead of std::thread as requested")
endif()

if (${USE_GLES})
  message("Using GL ES profile as requested")
endif()

# loads the include dirs of modules this one is dependent on and saves them to the given variable
function(load_dep_module_includes dest)
  set(MODULE_MASTER_DEPENDENCY_INCLUDES "")
  foreach(module ${MODULE_DEPS})
    get_property(MODULE_INC_DIR GLOBAL PROPERTY ${module}_INCLUDE_DIRS)
    list(APPEND MODULE_MASTER_DEPENDENCY_INCLUDES "${MODULE_INC_DIR}")
  endforeach()
  set(${dest} "${MODULE_MASTER_DEPENDENCY_INCLUDES}" PARENT_SCOPE)
endfunction()

# configures the module's top-level include dirs and saves them to the given variable
function(load_module_includes dest)
  # load this module's include dir
  set(ALL_INCLUDES "${PROJECT_SOURCE_DIR}/include")

  # load include dirs defined by module dependencies
  load_dep_module_includes(PARENT_INCLUDES)
  list(LENGTH PARENT_INCLUDES LEN)
  if(LEN GREATER 0)
    list(APPEND ALL_INCLUDES "${PARENT_INCLUDES}")
  endif()

  # load include dirs defined by this module
  list(LENGTH LOCAL_INC_DIRS LEN)
  if(LEN GREATER 0)
    list(APPEND ALL_INCLUDES "${LOCAL_INC_DIRS}")
  endif()

  # remove any duplicates
  list(REMOVE_DUPLICATES ALL_INCLUDES)

  set_property(GLOBAL PROPERTY ${PROJECT_NAME}_INCLUDE_DIRS "${ALL_INCLUDES}")
  set_property(GLOBAL PROPERTY INCLUDE_DIRECTORIES "${ALL_INCLUDES}")

  set(${dest} "${ALL_INCLUDES}" PARENT_SCOPE)
endfunction()

function(setup_submodule)
  project(${MODULE_NAME} C CXX)

  # source files
  file(GLOB C_FILES ${SRC_DIR}/*.c)
  file(GLOB CPP_FILES ${SRC_DIR}/*.cpp)
  file(GLOB H_FILES ${SRC_DIR}/*.h)
  file(GLOB HPP_FILES ${SRC_DIR}/*.hpp)

  # recursively load this module's include dirs
  load_module_includes(MODULE_INCLUDES)
  include_directories("${MODULE_INCLUDES}")
  get_property(GLOBAL_INCLUDES GLOBAL PROPERTY GLOBAL_INCLUDES)
  list(APPEND GLOBAL_INCLUDES "${MODULE_INCLUDES}")
  set_property(GLOBAL PROPERTY GLOBAL_INCLUDES "${GLOBAL_INCLUDES}")

  if(${USE_PTHREADS})
    add_definitions("-DUSE_PTHREADS")
  endif()

  if(${USE_GLES})
    add_definitions("-DUSE_GLES")
  endif()

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions("-D_ARGUS_DEBUG_MODE")
  endif()

  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    if(MSVC_VERSION LESS 1911)
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG:FULL /Od")
    else()
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG:FASTLINK /Od")
    endif()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
  else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  endif()

  add_library(${PROJECT_NAME} OBJECT ${C_FILES} ${CPP_FILES} ${H_FILES} ${HPP_FILES})

  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  get_property(GLOBAL_LIBS GLOBAL PROPERTY GLOBAL_LIBS)
  list(APPEND GLOBAL_LIBS "${LOCAL_LIBS}")
  set_property(GLOBAL PROPERTY GLOBAL_LIBS "${GLOBAL_LIBS}")

  string(REPLACE ";" " " MODULE_LIBS "${MODULE_DEPS}")
endfunction()

# transform the module paths to add the prefix
foreach(module ${MODULE_IDS})
  list(APPEND MODULE_PATHS "${MODULE_PATH_PREFIX}${module}")
endforeach()

set(SRC_DIR src)
set(INC_DIR include)

set(CMAKE_DIST_DIR ${CMAKE_BINARY_DIR}/dist)

set(CMAKE_LIB_OUT_DIR ${CMAKE_DIST_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${CMAKE_LIB_OUT_DIR})

# configure the copy headers task
set(HDR_TARGET "${PROJECT_NAME}_headers")
set(HDR_OUT_DIR "${CMAKE_DIST_DIR}/include/argus/")
add_custom_target(${HDR_TARGET})
add_custom_command(TARGET ${HDR_TARGET} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E
    make_directory ${HDR_OUT_DIR})
foreach(MODULE_ID ${MODULE_IDS})
endforeach()

foreach(module ${MODULE_IDS})
  add_subdirectory(${MODULE_PATH_PREFIX}${module})
  set(MODULE_LIBS "$<TARGET_OBJECTS:${module}>;${MODULE_LIBS}")

  # configure the copy headers task to include this module
  set(MODULE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/${MODULE_PATH_PREFIX}${module}/include/argus")
  file(GLOB_RECURSE HDR_FILES RELATIVE ${MODULE_INCLUDE_DIR} "*.h" "*.hpp")
  list(FILTER HDR_FILES EXCLUDE REGEX "^\\.\\..*")
  string(REGEX REPLACE "([^;]+)" "${MODULE_INCLUDE_DIR}/\\1" HDR_FILES "${HDR_FILES}")
  add_custom_command(TARGET ${HDR_TARGET} POST_BUILD
    COMMENT "${HDR_FILES}"
    COMMAND ${CMAKE_COMMAND} -E
      copy ${HDR_FILES} ${HDR_OUT_DIR})

  # add this module as a dependency of the copy headers task
  add_dependencies(${HDR_TARGET} ${module})
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
set_target_properties(${PROJECT_NAME}_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

# add libraries for linking
get_property(GLOBAL_LIBS GLOBAL PROPERTY GLOBAL_LIBS)
list(REMOVE_DUPLICATES GLOBAL_LIBS)
target_link_libraries(${PROJECT_NAME} ${GLOBAL_LIBS})

# add global include list
get_property(GLOBAL_INCLUDES GLOBAL PROPERTY GLOBAL_INCLUDES)
list(REMOVE_DUPLICATES GLOBAL_INCLUDES)
target_include_directories(${PROJECT_NAME} PUBLIC "${GLOBAL_INCLUDES}")
target_include_directories(${PROJECT_NAME}_static PUBLIC "${GLOBAL_INCLUDES}")

# make the copy headers target a dependency to force it to run
add_dependencies(${PROJECT_NAME} ${HDR_TARGET})
add_dependencies(${PROJECT_NAME}_static ${HDR_TARGET})
