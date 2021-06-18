set(MODULE_IDS "lowlevel;core;wm;ecs;resman;render;input")
set(DYN_MODULE_IDS "")
set(MODULE_PATH_PREFIX "modules/")

cmake_minimum_required(VERSION 3.8)
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW) # OpenGL library preference
endif()
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW) # use <package>_ROOT env var as hint
endif()

project(argus_root
        VERSION 0.1.0)

set(LIB_BASE_NAME argus)

set(DYN_MODULE_DIR "modules")

option(USE_PTHREADS "Use pthreads instead of std::thread" OFF)
option(BUILD_OPENGL_BACKEND "Build the OpenGL-based render backend" ON)
option(BUILD_OPENGLES_BACKEND "Build the OpenGL ES-based render backend" OFF)
option(BUILD_VULKAN_BACKEND "Build the Vulkan-based render backend" OFF)

#set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

##########################
# Dependency configuration
##########################

set(GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/libs/glfw")
set(ZLIB_SOURCE_DIR "${CMAKE_SOURCE_DIR}/libs/zlib")
set(PNG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/libs/libpng")
set(ARP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/libs/libarp")

# disable extra GLFW build steps
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

if(WIN32)
  include_directories("${CMAKE_SOURCE_DIR}/external/include")
endif()

# don't let libpng try to install itself
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
# configure libpng build steps
set(PNG_STATIC OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)

# include dir for generated headers which must be copied (configs)
set(TMP_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include.tmp")

# copy "generated" headers from dependencies to somewhere we can include easily
configure_file("${PNG_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt" "${TMP_INCLUDE_DIR}/libpng/pnglibconf.h")
configure_file("${ZLIB_SOURCE_DIR}/zconf.h.in" "${TMP_INCLUDE_DIR}/zlib/zconf.h")

if(NOT DEFINED UNIX)
  set(UNIX 0)
endif()

# set relevant build variables so the dependencies can be discovered
# note that we include the dirs for generated headers
set(GLFW_LIBRARY_BASE "glfw")
set(GLFW_LIBRARY "${GLFW_LIBRARY_BASE};${OPENGL_LIBRARIES}")
set(GLFW_INCLUDE_DIR "${GLFW_SOURCE_DIR}/include")

set(ZLIB_LIBRARY "zlib")
set(ZLIB_INCLUDE_DIR "${ZLIB_SOURCE_DIR};${TMP_INCLUDE_DIR}/zlib")

set(PNG_LIBRARY "png")
set(PNG_INCLUDE_DIR "${PNG_SOURCE_DIR};${TMP_INCLUDE_DIR}/libpng")

set(ARP_LIBRARY_BASE "arp")
set(ARP_LIBRARY "${ARP_LIBRARY_BASE};${ZLIB_LIBRARY}")
set(ARP_INCLUDE_DIR "${ARP_SOURCE_DIR}/include")

# add dependencies
add_subdirectory("${GLFW_SOURCE_DIR}")
add_subdirectory("${ZLIB_SOURCE_DIR}")
add_subdirectory("${PNG_SOURCE_DIR}")
add_subdirectory("${ARP_SOURCE_DIR}")

set_target_properties(zlibstatic PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(example PROPERTIES EXCLUDE_FROM_ALL TRUE)
if(TARGET example64)
  set_target_properties(example64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
set_target_properties(minigzip PROPERTIES EXCLUDE_FROM_ALL TRUE)
if(TARGET minigzip64)
  set_target_properties(minigzip64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
set_target_properties(pngfix PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(png-fix-itxt PROPERTIES EXCLUDE_FROM_ALL TRUE)

###########################
# /Dependency configuration
###########################

if(${USE_PTHREADS})
  message("Using pthreads instead of std::thread as requested")
endif()

if(${BUILD_OPENGL_BACKEND})
  message("OpenGL support will be built")
  list(APPEND DYN_MODULE_IDS "render_opengl")
endif()
if(${BUILD_OPENGLES_BACKEND})
  message(FATAL_ERROR "OpenGL ES support is not yet available")
endif()

if(${BUILD_VULKAN_BACKEND})
  message(FATAL_ERROR "Vulkan support is not yet available")
endif()

# disable the annoying (and wrong) class vs. struct warning
if(MSVC)
  add_compile_options("/wd4099")
endif()

# init task to copy dependency output
function(copy_dep_output PARENT_TARGET DEP_TARGET PREFIX)
  if(PREFIX MATCHES "^$")
    set(PREFIX ".")
  endif()
  
  add_custom_command(TARGET ${PARENT_TARGET} POST_BUILD 
    COMMAND "${CMAKE_COMMAND}" -E copy
      "$<TARGET_FILE:${DEP_TARGET}>"
      "${DIST_DIR}/lib/${PREFIX}/$<IF:$<BOOL:${MSVC}>,$<CONFIG>/,>$<TARGET_FILE_NAME:${DEP_TARGET}>"
    COMMENT "Copying '${DEP_TARGET}' dist output to output directory")
  if(WIN32)
    add_custom_command(TARGET ${PARENT_TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_LINKER_FILE:${DEP_TARGET}>"
        "${DIST_DIR}/lib/${PREFIX}/$<IF:$<BOOL:${MSVC}>,$<CONFIG>/,>$<TARGET_LINKER_FILE_NAME:${DEP_TARGET}>"
      COMMENT "Copying '${DEP_TARGET}' linker output to output directory")
  endif()
endfunction()

# loads the include dirs of modules this one is dependent on and saves them to the given variable
function(load_dep_module_includes DEST)
  set(MODULE_MASTER_DEPENDENCY_INCLUDES "")
  foreach(module ${MODULE_DEPS})
    get_property(MODULE_INC_DIR GLOBAL PROPERTY ${module}_INCLUDE_DIRS)
    list(APPEND MODULE_MASTER_DEPENDENCY_INCLUDES "${MODULE_INC_DIR}")
  endforeach()
  set(${DEST} "${MODULE_MASTER_DEPENDENCY_INCLUDES}" PARENT_SCOPE)
endfunction()

# configures the module's top-level include dirs and saves them to the given variable
function(load_module_includes DEST)
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

  set(${DEST} "${ALL_INCLUDES}" PARENT_SCOPE)
endfunction()

function(setup_submodule MODULE_NAME DYN)
  if(DYN)
    project("${MODULE_NAME}" C CXX)
  else()
    project(${MODULE_NAME} C CXX)
  endif()

  # source files
  file(GLOB_RECURSE C_FILES ${SRC_DIR}/*.c)
  file(GLOB_RECURSE CPP_FILES ${SRC_DIR}/*.cpp)
  file(GLOB_RECURSE H_FILES ${SRC_DIR}/*.h)
  file(GLOB_RECURSE HPP_FILES ${SRC_DIR}/*.hpp)

  # recursively load this module's include dirs
  load_module_includes(MODULE_INCLUDES)
  include_directories("${MODULE_INCLUDES}")

  if(NOT DYN)
    get_property(GLOBAL_INCLUDES GLOBAL PROPERTY GLOBAL_INCLUDES)
    list(APPEND GLOBAL_INCLUDES "${MODULE_INCLUDES}")
    set_property(GLOBAL PROPERTY GLOBAL_INCLUDES "${GLOBAL_INCLUDES}")
  endif()

  if(${USE_PTHREADS})
    add_definitions("-DUSE_PTHREADS")
  endif()

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions("-D_ARGUS_DEBUG_MODE")
  endif()

  if(WIN32)
    add_definitions("-DGLFW_DLL")
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
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Winit-self -Wold-style-cast -Woverloaded-virtual -Wuninitialized \
                         -Wmissing-declarations -Winit-self -Wshadow -pedantic -pedantic-errors -std=c++11")
    set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "-fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  endif()

  if(DYN)
    add_library(${PROJECT_NAME} SHARED ${C_FILES} ${CPP_FILES} ${H_FILES} ${HPP_FILES})

    # enable PIC
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    # export all symbols (required on Windows)
    set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

    # output to separate directory
    set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${DYN_MODULE_DIR}")

    set(ALL_PROJECT_LIBS ${LOCAL_LIBS})
    list(APPEND ALL_PROJECT_LIBS ${LIB_BASE_NAME})
    target_link_libraries(${PROJECT_NAME} ${ALL_PROJECT_LIBS})

    copy_dep_output("${PROJECT_NAME}" "${PROJECT_NAME}" "${DYN_MODULE_DIR}")
  else()
    add_library(${PROJECT_NAME} OBJECT ${C_FILES} ${CPP_FILES} ${H_FILES} ${HPP_FILES})

    get_property(GLOBAL_LIBS GLOBAL PROPERTY GLOBAL_LIBS)
    list(APPEND GLOBAL_LIBS "${LOCAL_LIBS}")
    set_property(GLOBAL PROPERTY GLOBAL_LIBS "${GLOBAL_LIBS}")

    string(REPLACE ";" " " MODULE_LIBS "${MODULE_DEPS}")
  endif()

  # set the C++ standard
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)

  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

set(SRC_DIR src)
set(INC_DIR include)

set(DIST_DIR ${CMAKE_BINARY_DIR}/dist)

set(LIB_OUT_DIR ${CMAKE_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${LIB_OUT_DIR})

# configure the copy headers task
set(HDR_TARGET "${LIB_BASE_NAME}_headers")
set(HDR_OUT_DIR "${DIST_DIR}/include/argus/")
add_custom_target(${HDR_TARGET})
add_custom_command(TARGET ${HDR_TARGET} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E
    make_directory ${HDR_OUT_DIR})

foreach(module ${MODULE_IDS})
  add_subdirectory(${MODULE_PATH_PREFIX}${module})
  set(MODULE_LIBS "$<TARGET_OBJECTS:${module}>;${MODULE_LIBS}")

  # configure the copy headers task to include this module
  set(MODULE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/${MODULE_PATH_PREFIX}${module}/include/argus")
  add_custom_command(TARGET ${HDR_TARGET} POST_BUILD
    COMMENT "Copying headers for module ${module}"
    COMMAND ${CMAKE_COMMAND} -E
      copy_directory ${MODULE_INCLUDE_DIR} ${HDR_OUT_DIR})

  # add this module as a dependency of the copy headers task
  add_dependencies(${HDR_TARGET} ${module})
endforeach()

foreach(module ${DYN_MODULE_IDS})
  add_subdirectory("${MODULE_PATH_PREFIX}${module}")
endforeach()

add_library(${LIB_BASE_NAME} SHARED ${MODULE_LIBS})

# set the linker language (since we're not directly including any sources)
set_target_properties(${LIB_BASE_NAME} PROPERTIES LINKER_LANGUAGE CXX)
# set the C++ standard
set_target_properties(${LIB_BASE_NAME} PROPERTIES CXX_STANDARD 11)
# enable PIC
set_target_properties(${LIB_BASE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
# export all symbols (required on Windows)
set_target_properties(${LIB_BASE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# add libraries for linking
get_property(GLOBAL_LIBS GLOBAL PROPERTY GLOBAL_LIBS)
list(REMOVE_DUPLICATES GLOBAL_LIBS)
target_link_libraries(${LIB_BASE_NAME} ${GLOBAL_LIBS})

# add global include list
get_property(GLOBAL_INCLUDES GLOBAL PROPERTY GLOBAL_INCLUDES)
list(REMOVE_DUPLICATES GLOBAL_INCLUDES)
target_include_directories(${LIB_BASE_NAME} PUBLIC "${GLOBAL_INCLUDES}")

foreach(module ${DYN_MODULE_IDS})
  add_dependencies("${module}" "${LIB_BASE_NAME}")
endforeach()

# make the copy headers target a dependency to force it to run
add_dependencies(${LIB_BASE_NAME} ${HDR_TARGET})

copy_dep_output("${LIB_BASE_NAME}" "${LIB_BASE_NAME}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${ZLIB_LIBRARY}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${PNG_LIBRARY}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${GLFW_LIBRARY_BASE}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${ARP_LIBRARY_BASE}" ".")
