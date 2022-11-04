set(ENGINE_LIBRARY_IDS "lowlevel;shadertools")
set(STATIC_MODULE_IDS "core;ecs;resman;wm;input;render;ui;sound;game2d")
set(DYNAMIC_MODULE_IDS "")

set(ENGINE_SRC_PATH "${CMAKE_SOURCE_DIR}/engine")
set(STATIC_MODULES_SRC_DIR "${ENGINE_SRC_PATH}/static")
set(DYNAMIC_MODULES_SRC_DIR "${ENGINE_SRC_PATH}/dynamic")
set(ENGINE_LIBS_SRC_DIR "${ENGINE_SRC_PATH}/libs")

cmake_minimum_required(VERSION 3.17)
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW) # use <package>_ROOT env var as hint
endif()

project(argus_root
        LANGUAGES C CXX
        VERSION 0.1.0)

if("${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}" STREQUAL "10.0.17763.0")
  message(FATAL_ERROR "Compilation against Windows SDK version 10.0.17763 is broken due to a bug in the SDK headers. \
  Please use a different SDK version.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(PROJECT_CXX_VERSION 17)
set(PROJECT_CXX_EXTENSIONS NO)

set(LIB_BASE_NAME argus)

set(DYN_MODULE_DIR "modules")

set(ROOT_PROJECT_DIR "${PROJECT_SOURCE_DIR}")

set(RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-target")

set(GENERATED_BASE_DIR "${PROJECT_BINARY_DIR}/generated")
set(STATIC_MODULES_GENERATED_DIR "${GENERATED_BASE_DIR}/static")
set(DYNAMIC_MODULES_GENERATED_DIR "${GENERATED_BASE_DIR}/dynamic")
set(ENGINE_LIBS_GENERATED_DIR "${GENERATED_BASE_DIR}/libs")

option(BUILD_OPENGL_BACKEND "Build the OpenGL-based render backend" ON)
option(BUILD_OPENGLES_BACKEND "Build the OpenGL ES-based render backend" ON)
option(BUILD_VULKAN_BACKEND "Build the Vulkan-based render backend" ON)

option(USE_ASAN "Build with AddressSanitizer support" OFF)

include("${ROOT_PROJECT_DIR}/cmake/UtilityFunctions.cmake")

#####################
# CMake configuration
#####################

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_MODULE_PROJECT_DIR ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(ExternalProject)

######################
# /CMake configuration
######################

########################
# Compiler configuration
########################

include("${ROOT_PROJECT_DIR}/cmake/ConfigureCompileFlags.cmake")

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od" CACHE STRING "" FORCE)
  if(MSVC_VERSION LESS 1911)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG:FULL /Od" CACHE STRING "" FORCE)
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG /Od" CACHE STRING "" FORCE)
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2" CACHE STRING "" FORCE)
else()
  if(USE_ASAN)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer"
        CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address"
        CACHE STRING "" FORCE)
  endif()

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3" CACHE STRING "" FORCE)
endif()

#########################
# /Compiler configuration
#########################

##########################
# Dependency configuration
##########################

include("${ROOT_PROJECT_DIR}/cmake/ConfigureExternalTargets.cmake")

set(PROJECT_TYPE_STATIC "static")
set(PROJECT_TYPE_DYNAMIC "dynamic")
set(PROJECT_TYPE_LIBRARY "library")

set(SOURCE_DIR_NAME "src")
set(INCLUDE_DIR_NAME "include")

set(MODULE_DEFS_HPP_IN "${STATIC_MODULES_SRC_DIR}/core/${INCLUDE_DIR_NAME}/internal/core/module_defs.hpp.in")
set(MODULE_DEFS_HPP_OUT "${STATIC_MODULES_GENERATED_DIR}/core/${INCLUDE_DIR_NAME}/internal/core/module_defs.hpp")

###########################
# /Dependency configuration
###########################

#######################
# Feature configuration
#######################

if(${BUILD_OPENGL_BACKEND})
  message("OpenGL support will be built")
  list(APPEND DYNAMIC_MODULE_IDS "render_opengl")
  else()
    message("OpenGL support will not be built")
endif()

if(${BUILD_OPENGLES_BACKEND})
  message("OpenGL ES support will be built")
  list(APPEND DYNAMIC_MODULE_IDS "render_opengles")
else()
  message("OpenGL ES support will not be built")
endif()

if(${BUILD_VULKAN_BACKEND})
  message("Vulkan support will be built")
  list(APPEND DYNAMIC_MODULE_IDS "render_vulkan")
  else()
    message("Vulkan support will not be built")
endif()

########################
# /Feature configuration
########################

#######################
# arptool configuration
#######################

message("Configuring arptool")
set(arptool_wd "${CMAKE_BINARY_DIR}/tooling/arptool")

set(arptool_exe_dir "${CMAKE_BINARY_DIR}/tooling/arptool")

execute_process(COMMAND "${CMAKE_COMMAND}" -E make_directory "${arptool_wd}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-G" "${CMAKE_GENERATOR}"
                        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                        "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
                        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${arptool_exe_dir}"
                        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${arptool_exe_dir}"
                        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${arptool_exe_dir}"
                        "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
                        "${CMAKE_SOURCE_DIR}/external/tooling/arptool"
                WORKING_DIRECTORY "${arptool_wd}"
                RESULT_VARIABLE CMD_RES)
if(CMD_RES)
  message(FATAL_ERROR "    Failed to configure arptool: ${CMD_RES}")
endif()

set(arptool_exe "${arptool_exe_dir}/arptool${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_command(OUTPUT "${arptool_exe}"
                  COMMAND "${CMAKE_COMMAND}" "--build" "."
                  DEPENDS "${arptool_source_files}"
                  WORKING_DIRECTORY "${arptool_wd}")
add_custom_target("arptool" DEPENDS "${arptool_exe}")

########################
# /arptool configuration
########################

include("${ROOT_PROJECT_DIR}/cmake/ConfigureModule.cmake")

set(DIST_DIR ${CMAKE_BINARY_DIR}/dist)

set(LIB_OUT_DIR ${CMAKE_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${LIB_OUT_DIR})

# configure the copy headers task
set(HDR_TARGET "${LIB_BASE_NAME}_headers")
set(HDR_OUT_DIR "${DIST_DIR}/${INCLUDE_DIR_NAME}/argus/")
add_custom_target(${HDR_TARGET})
add_custom_command(TARGET ${HDR_TARGET} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E
    make_directory ${HDR_OUT_DIR})

foreach(id ${ENGINE_LIBRARY_IDS})
  _argus_configure_module("${ENGINE_LIBS_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}" "${PROJECT_CXX_VERSION}"
                          "${PROJECT_CXX_EXTENSIONS}" "${arptool_exe}")
endforeach()

foreach(id ${STATIC_MODULE_IDS})
  _argus_configure_module("${STATIC_MODULES_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}" "${PROJECT_CXX_VERSION}"
                          "${PROJECT_CXX_EXTENSIONS}" "${arptool_exe}")
endforeach()

# IMPORTANT: these should be configured last so that they can avoid compiling in
# libraries already present in the base library
foreach(id ${DYNAMIC_MODULE_IDS})
  _argus_configure_module("${DYNAMIC_MODULES_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}" "${PROJECT_CXX_VERSION}"
                          "${PROJECT_CXX_EXTENSIONS}" "${arptool_exe}")
endforeach()

# configure module dependency information

get_property(DEP_EDGES GLOBAL PROPERTY "DEPENDENCY_GRAPH_EDGES")
_argus_topo_sort("${STATIC_MODULE_IDS}" "${DEP_EDGES}" STATIC_MODULE_IDS)

set(STATIC_MODULE_ID_SET_LIST ${STATIC_MODULE_IDS})
list(TRANSFORM STATIC_MODULE_ID_SET_LIST REPLACE "^(.+)$" "\"\\1\"")
list(JOIN STATIC_MODULE_ID_SET_LIST ",\n        " STATIC_MODULE_ID_SET)

set(STATIC_MODULE_DEFS_LIST "")
foreach(id ${STATIC_MODULE_IDS})
  set(deps_list ${MODULE_${id}_DEPS})
  list(TRANSFORM deps_list REPLACE "^(.+)$" "\"\\1\"")
  list(JOIN deps_list ", " deps)
  list(APPEND STATIC_MODULE_DEFS_LIST "{\"${id}\", {${deps}}, update_lifecycle_${id}}")
endforeach()
list(JOIN STATIC_MODULE_DEFS_LIST ",\n        " STATIC_MODULE_DEFS)

list(LENGTH STATIC_MODULE_IDS MODULE_COUNT)

set(MODULE_LIFECYCLE_FNS_LIST "${STATIC_MODULE_IDS}")
list(TRANSFORM MODULE_LIFECYCLE_FNS_LIST REPLACE "^(.+)$" "extern void update_lifecycle_\\1(LifecycleStage stage)\\\;")
list(JOIN MODULE_LIFECYCLE_FNS_LIST "\n    " MODULE_LIFECYCLE_FNS)

configure_file("${MODULE_DEFS_HPP_IN}" "${MODULE_DEFS_HPP_OUT}")

list(REMOVE_DUPLICATES STATIC_MODULE_LIBS)
set(RUST_LIBS_FOR_LINKING "")
foreach(lib ${STATIC_MODULE_LIBS})
  get_property(lib_is_external TARGET "${lib}" PROPERTY IS_EXTERNAL)
  if(${lib_is_external})
    get_property(lib_is_rust TARGET "${lib}" PROPERTY IS_RUST)

    if(${lib_is_rust})
      list(APPEND RUST_LIBS_FOR_LINKING "${lib}")
    endif()
  else()
    list(APPEND STATIC_MODULE_LIB_OBJS "$<TARGET_OBJECTS:${lib}>")
  endif()
endforeach()
add_library(${LIB_BASE_NAME} SHARED ${STATIC_MODULE_LIB_OBJS})

_argus_set_compile_flags(${LIB_BASE_NAME})

# set the linker language (since we're not directly including any sources)
set_target_properties(${LIB_BASE_NAME} PROPERTIES LINKER_LANGUAGE CXX)
# set the C++ standard
set_target_properties(${LIB_BASE_NAME} PROPERTIES CXX_STANDARD "${PROJECT_CXX_VERSION}")
set_target_properties(${LIB_BASE_NAME} PROPERTIES CXX_EXTENSIONS "${PROJECT_CXX_EXTENSIONS}")
set_target_properties(${LIB_BASE_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
# enable PIC
set_target_properties(${LIB_BASE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
# export all symbols (required on Windows)
set_target_properties(${LIB_BASE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# add libraries for linking
get_property(COMBINED_TARGET_LINKER_DEPS GLOBAL PROPERTY COMBINED_TARGET_LINKER_DEPS)
list(REMOVE_DUPLICATES COMBINED_TARGET_LINKER_DEPS)
target_link_libraries(${LIB_BASE_NAME} "${COMBINED_TARGET_LINKER_DEPS}")

foreach(lib ${RUST_LIBS_FOR_LINKING})
  target_link_libraries(${LIB_BASE_NAME}
    debug "${RUST_TARGET_DIR}/${lib}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    optimized "${RUST_TARGET_DIR}/${lib}/release/${CMAKE_STATIC_LIBRARY_PREFIX}${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}")
endforeach()

# add global include list
get_property(COMBINED_TARGET_INCLUDES GLOBAL PROPERTY COMBINED_TARGET_INCLUDES)
list(REMOVE_DUPLICATES COMBINED_TARGET_INCLUDES)
target_include_directories(${LIB_BASE_NAME} PUBLIC "${COMBINED_TARGET_INCLUDES}")

foreach(module ${DYNAMIC_MODULE_IDS})
  add_dependencies("${module}" "${LIB_BASE_NAME}")
endforeach()

# make the copy headers target a dependency to force it to run
add_dependencies(${LIB_BASE_NAME} ${HDR_TARGET})

_argus_copy_dep_output("${DIST_DIR}" "${LIB_BASE_NAME}" "${LIB_BASE_NAME}" ".")
_argus_copy_dep_output("${DIST_DIR}" "${LIB_BASE_NAME}" "${ZLIB_LIBRARY}" ".")
_argus_copy_dep_output("${DIST_DIR}" "${LIB_BASE_NAME}" "${PNG_LIBRARY}" ".")
_argus_copy_dep_output("${DIST_DIR}" "${LIB_BASE_NAME}" "${GLFW_LIBRARY_BASE}" ".")
_argus_copy_dep_output("${DIST_DIR}" "${LIB_BASE_NAME}" "${ARP_LIBRARY_BASE}" ".")
