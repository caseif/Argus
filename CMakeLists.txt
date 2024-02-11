include(CMakeDependentOption)

set(ENGINE_LIBRARY_IDS "lowlevel;shadertools")
set(STATIC_MODULE_IDS "core;ecs;resman;"
        "scripting;scripting_lua;"
        #"scripting_angelscript;"
        "wm;input;render;ui;sound;game2d")
set(DYNAMIC_MODULE_IDS "")
set(EXE_MODULE_IDS "bootstrap")
set(AUX_MODULE_IDS "lowlevel_rustabi;core_rustabi")

set(ARGUS_ROOT_DIR "${CMAKE_SOURCE_DIR}")
set(ENGINE_SRC_PATH "${CMAKE_SOURCE_DIR}/engine")
set(STATIC_MODULES_SRC_DIR "${ENGINE_SRC_PATH}/static")
set(DYNAMIC_MODULES_SRC_DIR "${ENGINE_SRC_PATH}/dynamic")
set(ENGINE_LIBS_SRC_DIR "${ENGINE_SRC_PATH}/libs")
set(EXES_SRC_DIR "${ENGINE_SRC_PATH}/exe")
set(AUX_MODULES_SRC_DIR "${ENGINE_SRC_PATH}/auxiliary")

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(GCC TRUE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(PROJECT_C_VERSION 11)
set(PROJECT_C_EXTENSIONS NO)
set(PROJECT_CXX_VERSION 17)
set(PROJECT_CXX_EXTENSIONS NO)

set(LIB_BASE_NAME "argus")
set(ARGUS_LIBRARY "${LIB_BASE_NAME}")

set(BIN_PREFIX "bin")
set(LIB_PREFIX "lib")
set(DYN_MODULE_PREFIX "${LIB_PREFIX}/modules")

set(ROOT_PROJECT_DIR "${PROJECT_SOURCE_DIR}")

set(RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-target")

set(GENERATED_BASE_DIR "${PROJECT_BINARY_DIR}/generated")
set(STATIC_MODULES_GENERATED_DIR "${GENERATED_BASE_DIR}/static")
set(DYNAMIC_MODULES_GENERATED_DIR "${GENERATED_BASE_DIR}/dynamic")
set(ENGINE_LIBS_GENERATED_DIR "${GENERATED_BASE_DIR}/libs")
set(EXES_GENERATED_DIR "${GENERATED_BASE_DIR}/exe")
set(AUX_MODULES_GENERATED_DIR "${GENERATED_BASE_DIR}/auxiliary")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(DEF_USE_SYSTEM_SDL2 OFF)
  set(DEF_USE_SYSTEM_ZLIB OFF)
  set(DEF_USE_SYSTEM_PNG OFF)
  set(DEF_USE_SYSTEM_LUA OFF)
  set(DEF_USE_SYSTEM_JSON OFF)
else()
  set(DEF_USE_SYSTEM_SDL2 ON)
  set(DEF_USE_SYSTEM_ZLIB ON)
  set(DEF_USE_SYSTEM_PNG ON)
  set(DEF_USE_SYSTEM_LUA ON)
  set(DEF_USE_SYSTEM_JSON ON)
endif()

option(ENABLE_CTEST "Enable testing" ON)

option(BUILD_OPENGL_BACKEND "Build the OpenGL-based render backend" ON)
option(BUILD_OPENGL_LEGACY_BACKEND "Build the legacy OpenGL-based render backend" ON)
option(BUILD_OPENGLES_BACKEND "Build the OpenGL ES-based render backend" ON)
option(BUILD_VULKAN_BACKEND "Build the Vulkan-based render backend" ON)

option(NO_SYSTEM_LIBS "Do not use any system-provided libraries" OFF)
cmake_dependent_option(USE_SYSTEM_SDL2 "Use system-provided SDL2 library and headers"
        "${DEF_USE_SYSTEM_SDL2}"
        "NOT NO_SYSTEM_LIBS" OFF)
cmake_dependent_option(USE_SYSTEM_ZLIB "Use system-provided zlib library and headers"
        "${DEF_USE_SYSTEM_ZLIB}"
        "NOT NO_SYSTEM_LIBS" OFF)
cmake_dependent_option(USE_SYSTEM_PNG "Use system-provided libpng library and headers"
        "${DEF_USE_SYSTEM_PNG}"
        "NOT NO_SYSTEM_LIBS" OFF)
cmake_dependent_option(USE_SYSTEM_LUA "Use system-provided Lua library and headers"
        "${DEF_USE_SYSTEM_LUA}"
        "NOT NO_SYSTEM_LIBS" OFF)
cmake_dependent_option(USE_SYSTEM_JSON "Use system-provided JSON (nlohmann-json) library and headers"
        "${DEF_USE_SYSTEM_JSON}"
        "NOT NO_SYSTEM_LIBS" OFF)

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

#####################
# CTest configuration
#####################

# disable tests for external subprojects that don't provide an option
# (currently just zlib)
configure_file(${CMAKE_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_BINARY_DIR})

######################
# /CTest configuration
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

#########################
# /Compiler configuration
#########################

##########################
# Dependency configuration
##########################

include("${ROOT_PROJECT_DIR}/cmake/ConfigureExternalTargets.cmake")

set(MODULE_TYPE_STATIC "static")
set(MODULE_TYPE_DYNAMIC "dynamic")
set(MODULE_TYPE_LIBRARY "library")
set(MODULE_TYPE_EXE "exe")
set(MODULE_TYPE_AUX "aux")

set(SOURCE_DIR_NAME "src")
set(INCLUDE_DIR_NAME "include")

set(MODULE_DEFS_HPP_IN "${STATIC_MODULES_SRC_DIR}/core/${INCLUDE_DIR_NAME}/internal/core/module_defs.hpp.in")
set(MODULE_DEFS_HPP_OUT "${STATIC_MODULES_GENERATED_DIR}/core/${INCLUDE_DIR_NAME}/internal/core/module_defs.hpp")

if(APPLE)
  find_library(COREFOUNDATION_LIBRARIES CoreFoundation)
elseif(UNIX AND NOT APPLE)
  check_library_exists(atomic __atomic_fetch_add_8 "" HAVE_LIBATOMIC)
  if(HAVE_LIBATOMIC)
    set(ATOMIC_LIBRARIES "atomic")
  else()
    message(FATAL "Unable to find libatomic")
  endif()
endif()

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

if(${BUILD_OPENGL_LEGACY_BACKEND})
  message("OpenGL (legacy) support will be built")
  list(APPEND DYNAMIC_MODULE_IDS "render_opengl_legacy")
else()
  message("OpenGL (legacy) support will not be built")
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

set(ARPTOOL_EXE "${arptool_exe_dir}/arptool${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_command(OUTPUT "${ARPTOOL_EXE}"
        COMMAND "${CMAKE_COMMAND}" "--build" "."
        DEPENDS "${arptool_source_files}"
        WORKING_DIRECTORY "${arptool_wd}")
add_custom_target("arptool" DEPENDS "${ARPTOOL_EXE}")

########################
# /arptool configuration
########################

####################
# Rust configuration
####################

if(MINGW)
  find_program(RUSTC_EXE NAMES rustc)

  set(CARGO_ARCH "")
  set(CARGO_TARGET "")
  if(NOT RUSTC_EXE STREQUAL "RUSTC_EXE-NOTFOUND")
    execute_process(
            COMMAND "${RUSTC_EXE}" "--version" "--verbose"
            OUTPUT_VARIABLE RUSTC_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REGEX MATCH "host: ([A-Za-z0-9_]+)\\-" _ ${RUSTC_VERSION})
    set(CARGO_ARCH "${CMAKE_MATCH_1}")

    set(Rust_CARGO_TARGET "${CARGO_ARCH}-pc-windows-gnu")
  endif()
endif()

add_subdirectory("external/tooling/corrosion")

include("${ROOT_PROJECT_DIR}/cmake/ConfigureModule.cmake")

set(DIST_DIR ${CMAKE_BINARY_DIR}/dist)

set(LIB_OUT_DIR ${CMAKE_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_DIRECTORY ${LIB_OUT_DIR})

set(ARGUS_INCLUDE_DIRS "")

# configure the copy headers task
set(HDR_TARGET "${LIB_BASE_NAME}_headers")
set(HDR_OUT_DIR "${DIST_DIR}/${INCLUDE_DIR_NAME}/argus/")
add_custom_target(${HDR_TARGET})
add_custom_command(TARGET ${HDR_TARGET} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        make_directory ${HDR_OUT_DIR})

foreach(id ${ENGINE_LIBRARY_IDS})
  _argus_configure_module("${ENGINE_LIBS_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}"
          "${PROJECT_C_VERSION}" "${PROJECT_C_EXTENSIONS}"
          "${PROJECT_CXX_VERSION}" "${PROJECT_CXX_EXTENSIONS}"
          "${ARPTOOL_EXE}")
endforeach()

foreach(id ${STATIC_MODULE_IDS})
  _argus_configure_module("${STATIC_MODULES_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}"
          "${PROJECT_C_VERSION}" "${PROJECT_C_EXTENSIONS}"
          "${PROJECT_CXX_VERSION}" "${PROJECT_CXX_EXTENSIONS}"
          "${ARPTOOL_EXE}")
endforeach()

# IMPORTANT: these should be configured last so that they can avoid compiling in
# libraries already present in the base library
foreach(id ${DYNAMIC_MODULE_IDS})
  _argus_configure_module("${DYNAMIC_MODULES_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}"
          "${PROJECT_C_VERSION}" "${PROJECT_C_EXTENSIONS}"
          "${PROJECT_CXX_VERSION}" "${PROJECT_CXX_EXTENSIONS}"
          "${ARPTOOL_EXE}")
endforeach()

foreach(id ${EXE_MODULE_IDS})
  _argus_configure_module("${EXES_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}"
          "${PROJECT_C_VERSION}" "${PROJECT_C_EXTENSIONS}"
          "${PROJECT_CXX_VERSION}" "${PROJECT_CXX_EXTENSIONS}"
          "${ARPTOOL_EXE}")
endforeach()

foreach(id ${AUX_MODULE_IDS})
  _argus_configure_module("${AUX_MODULES_SRC_DIR}/${id}" "${ROOT_PROJECT_DIR}"
          "${PROJECT_C_VERSION}" "${PROJECT_C_EXTENSIONS}"
          "${PROJECT_CXX_VERSION}" "${PROJECT_CXX_EXTENSIONS}"
          "${ARPTOOL_EXE}")
endforeach()

# configure module dependency information

get_property(DEP_EDGES GLOBAL PROPERTY DEPENDENCY_GRAPH_EDGES)
_argus_topo_sort("${STATIC_MODULE_IDS};${AUX_MODULE_IDS}" "${DEP_EDGES}" SORTED_MODULE_IDS)
foreach(aux_mod "${AUX_MODULE_IDS}")
  list(REMOVE_ITEM SORTED_MODULE_IDS "${aux_mod}")
endforeach()

set(STATIC_MODULE_ID_SET_LIST ${STATIC_MODULE_IDS})
list(TRANSFORM STATIC_MODULE_ID_SET_LIST REPLACE "^(.+)$" "\"\\1\"")
list(JOIN STATIC_MODULE_ID_SET_LIST ",\n        " STATIC_MODULE_ID_SET)

set(STATIC_MODULE_DEFS_LIST "")
set(MODULE_COUNT 0)
foreach(id ${SORTED_MODULE_IDS})
  if(NOT id IN_LIST STATIC_MODULE_IDS)
    continue()
  endif()

  set(deps_list ${MODULE_${id}_DEPS})
  list(TRANSFORM deps_list REPLACE "^(.+)$" "\"\\1\"")
  list(JOIN deps_list ", " deps)
  list(APPEND STATIC_MODULE_DEFS_LIST "{\"${id}\", {${deps}}, reinterpret_cast<LifecycleUpdateCallback>(reinterpret_cast<void *>(update_lifecycle_${id}))}")

  MATH(EXPR MODULE_COUNT "${MODULE_COUNT}+1")
endforeach()
list(JOIN STATIC_MODULE_DEFS_LIST ",\n        " STATIC_MODULE_DEFS)

set(MODULE_LIFECYCLE_FNS_LIST "${SORTED_MODULE_IDS}")
list(TRANSFORM MODULE_LIFECYCLE_FNS_LIST REPLACE "^(.+)$" "extern void update_lifecycle_\\1(std::underlying_type<argus::LifecycleStage> stage)\\\;")
list(JOIN MODULE_LIFECYCLE_FNS_LIST "\n    " MODULE_LIFECYCLE_FNS)

configure_file("${MODULE_DEFS_HPP_IN}" "${MODULE_DEFS_HPP_OUT}")

list(REMOVE_DUPLICATES STATIC_MODULE_LIBS)
set(RUST_LIBS_FOR_LINKING_STATIC "")
foreach(lib ${STATIC_MODULE_LIBS})
  get_property(lib_is_external TARGET "${lib}" PROPERTY IS_EXTERNAL)
  if(${lib_is_external})
    get_property(lib_is_rust TARGET "${lib}" PROPERTY IS_RUST)

    if(${lib_is_rust})
      list(APPEND RUST_LIBS_FOR_LINKING_STATIC "${lib}")
    endif()
  else()
    list(APPEND STATIC_MODULE_LIB_OBJS "$<TARGET_OBJECTS:${lib}>")
  endif()
endforeach()
add_library(${ARGUS_LIBRARY} SHARED ${STATIC_MODULE_LIB_OBJS})

_argus_set_compile_flags(${ARGUS_LIBRARY})

# set the linker language (since we're not directly including any sources)
set_target_properties(${ARGUS_LIBRARY} PROPERTIES LINKER_LANGUAGE CXX)
# set the C++ standard
set_target_properties(${ARGUS_LIBRARY} PROPERTIES CXX_STANDARD "${PROJECT_CXX_VERSION}")
set_target_properties(${ARGUS_LIBRARY} PROPERTIES CXX_EXTENSIONS "${PROJECT_CXX_EXTENSIONS}")
set_target_properties(${ARGUS_LIBRARY} PROPERTIES CXX_STANDARD_REQUIRED ON)
# enable PIC
set_target_properties(${ARGUS_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE ON)
# export all symbols (required on Windows)
set_target_properties(${ARGUS_LIBRARY} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
# set params for shared library output name
set_target_properties("${ARGUS_LIBRARY}" PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties("${ARGUS_LIBRARY}" PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties("${ARGUS_LIBRARY}" PROPERTIES DEBUG_POSTFIX "d")

# add libraries for linking
get_property(COMBINED_TARGET_LINKER_DEPS GLOBAL PROPERTY COMBINED_TARGET_LINKER_DEPS)
list(REMOVE_DUPLICATES COMBINED_TARGET_LINKER_DEPS)
target_link_libraries(${ARGUS_LIBRARY} "${COMBINED_TARGET_LINKER_DEPS}")

target_link_libraries(${ARGUS_LIBRARY} ${RUST_LIBS_FOR_LINKING_STATIC})
if(MINGW AND NOT "${RUST_LIBS_FOR_LINKING_STATIC}" STREQUAL "")
  target_link_libraries(${ARGUS_LIBRARY} "bcrypt" "ntdll" "Userenv" "ws2_32")
endif()

# add global include list
get_property(COMBINED_TARGET_INCLUDES GLOBAL PROPERTY COMBINED_TARGET_INCLUDES)
list(REMOVE_DUPLICATES COMBINED_TARGET_INCLUDES)
target_include_directories(${ARGUS_LIBRARY} PUBLIC "${COMBINED_TARGET_INCLUDES}")

foreach(module ${DYNAMIC_MODULE_IDS})
  add_dependencies("${module}" "${ARGUS_LIBRARY}")
endforeach()

# make the copy headers target a dependency to force it to run
add_dependencies(${ARGUS_LIBRARY} ${HDR_TARGET})

_argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${ARGUS_LIBRARY}" PREFIX "${LIB_PREFIX}")
if(TARGET "${ZLIB_TARGET}")
  _argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${ZLIB_TARGET}" PREFIX "${LIB_PREFIX}")
endif()
if(TARGET "${PNG_TARGET}")
  _argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${PNG_TARGET}" PREFIX "${LIB_PREFIX}")
endif()
if(TARGET "${SDL2_TARGET}")
  _argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${SDL2_TARGET}" PREFIX "${LIB_PREFIX}")
endif()
if(TARGET "${LUA_TARGET}")
  _argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${LUA_TARGET}" PREFIX "${LIB_PREFIX}")
endif()
_argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${GLSLANG_LIBRARY}" PREFIX "${LIB_PREFIX}")
_argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${SPIRV_CROSS_LIBRARY}" PREFIX "${LIB_PREFIX}")
#_argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${ANGELSCRIPT_LIBRARIES}" PREFIX "${LIB_PREFIX}")
_argus_copy_dep_output(DEST_DIR "${DIST_DIR}" TARGET "${ARP_LIBRARY}" PREFIX "${LIB_PREFIX}")

if(ENABLE_CTEST)
  enable_testing()
endif()

add_subdirectory("${PROJECT_SOURCE_DIR}/test")
