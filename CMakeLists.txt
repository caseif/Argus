set(ENGINE_LIBRARY_IDS "lowlevel")
set(STATIC_MODULE_IDS "core;ecs;resman;wm;input;render;ui;sound;game2d")
set(DYNAMIC_MODULE_IDS "")

set(ENGINE_SRC_PATH "${CMAKE_SOURCE_DIR}/engine")
set(STATIC_MODULES_SRC_DIR "${ENGINE_SRC_PATH}/static")
set(DYNAMIC_MODULES_SRC_DIR "${ENGINE_SRC_PATH}/dynamic")
set(ENGINE_LIBS_SRC_DIR "${ENGINE_SRC_PATH}/libs")

cmake_minimum_required(VERSION 3.12)
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

set(GENERATED_BASE_DIR "${PROJECT_BINARY_DIR}/generated")
set(STATIC_MODULES_GENERATED_DIR "${GENERATED_BASE_DIR}/static")
set(DYNAMIC_MODULES_GENERATED_DIR "${GENERATED_BASE_DIR}/dynamic")
set(ENGINE_LIBS_GENERATED_DIR "${GENERATED_BASE_DIR}/libs")

option(BUILD_OPENGL_BACKEND "Build the OpenGL-based render backend" ON)
option(BUILD_OPENGLES_BACKEND "Build the OpenGL ES-based render backend" ON)
option(BUILD_VULKAN_BACKEND "Build the Vulkan-based render backend" ON)

option(USE_ASAN "Build with AddressSanitizer support" OFF)

###########################
# Module property constants
###########################

set(MODULE_PROP_NAME "name")
set(MODULE_PROP_TYPE "type")
set(MODULE_PROP_LANGS "languages")
set(MODULE_PROP_INCLUDE_DIRS "include_dirs")
set(MODULE_PROP_MOD_DEPS "engine_module_deps")
set(MODULE_PROP_LIB_DEPS "engine_library_deps")
set(MODULE_PROP_LINKER_DEPS "linker_deps")
set(MODULE_PROP_LINKER_DEPS_MSVC "linker_deps_msvc")
set(MODULE_PROP_LINKER_DEPS_GCC "linker_deps_gcc")
set(MODULE_PROP_REQUIRED_PACKAGES "required_packages")
set(MODULE_PROP_OPTIONAL_PACKAGES "optional_packages")
set(MODULE_PROP_DEFINITIONS "definitions")

#####################
# CMake configuration
#####################

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_MODULE_PROJECT_DIR ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

######################
# /CMake configuration
######################

########################
# Compiler configuration
########################

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4996" CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od" CACHE STRING "" FORCE)
  if(MSVC_VERSION LESS 1911)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG:FULL /Od" CACHE STRING "" FORCE)
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG /Od" CACHE STRING "" FORCE)
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2" CACHE STRING "" FORCE)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wold-style-cast -Woverloaded-virtual -Wuninitialized \
                       -Wconversion -Wno-error=conversion -Wno-error=sign-conversion"
      CACHE STRING "" FORCE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
      OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 10))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=mismatched-tags" CACHE STRING "" FORCE)
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # enable pedantic errors, but with a few exceptions:
  #   - pasting , and __va_args__ so we can have a nice assert macro
  #   - anonymous structs, since there's no good standard alternative
  #   - zero-length arrays, since again there's no good standard alternative
  # we also only enable pedantic errors for Clang because GCC apparently lacks the ability to configure exceptions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -Wno-gnu-zero-variadic-macro-arguments \
                         -Wno-gnu-anonymous-struct -Wno-zero-length-array")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-variadic-macros")
  endif()

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

set(EXT_LIBS_DIR "${CMAKE_SOURCE_DIR}/external/libs")
set(EXT_SPECS_DIR "${CMAKE_SOURCE_DIR}/external/specs")

set(GLFW_SOURCE_DIR "${EXT_LIBS_DIR}/glfw")
set(ZLIB_SOURCE_DIR "${EXT_LIBS_DIR}/zlib")
set(PNG_SOURCE_DIR "${EXT_LIBS_DIR}/libpng")
set(JSON_SOURCE_DIR "${EXT_LIBS_DIR}/json")
set(ARP_SOURCE_DIR "${EXT_LIBS_DIR}/libarp")

# disable extra GLFW build steps
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# don't let libpng try to install itself
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
# configure libpng build steps
set(PNG_STATIC OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)

set(LIBARP_FEATURE_PACK OFF CACHE BOOL "" FORCE)
set(LIBARP_USER_MAPPINGS "${PROJECT_SOURCE_DIR}/res/arp_custom_mappings.csv" CACHE STRING "" FORCE)

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
set(GLFW_LIBRARY "${GLFW_LIBRARY_BASE}")
set(GLFW_INCLUDE_DIR "${GLFW_SOURCE_DIR}/include")

set(ZLIB_LIBRARY "zlib")
set(ZLIB_INCLUDE_DIR "${ZLIB_SOURCE_DIR};${TMP_INCLUDE_DIR}/zlib")

set(PNG_LIBRARY "png")
set(PNG_INCLUDE_DIR "${PNG_SOURCE_DIR};${TMP_INCLUDE_DIR}/libpng")

set(JSON_INCLUDE_DIR "${JSON_SOURCE_DIR}/include")

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
endif()

if(${BUILD_OPENGLES_BACKEND})
  message("OpenGL ES support will be built")
  list(APPEND DYNAMIC_MODULE_IDS "render_opengles")
endif()

if(${BUILD_VULKAN_BACKEND})
  message("Vulkan support will be built")
  list(APPEND DYNAMIC_MODULE_IDS "render_vulkan")
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

# init task to copy dependency output
function(copy_dep_output PARENT_TARGET DEP_TARGET PREFIX)
  if(PREFIX MATCHES "^$")
    set(PREFIX ".")
  endif()

  add_custom_command(TARGET ${PARENT_TARGET} POST_BUILD 
    COMMAND "${CMAKE_COMMAND}" -E copy
      "$<TARGET_FILE:${DEP_TARGET}>"
      "${DIST_DIR}/lib/${PREFIX}/$<TARGET_FILE_NAME:${DEP_TARGET}>"
    COMMENT "Copying '${DEP_TARGET}' dist output to output directory")
  if(WIN32)
    add_custom_command(TARGET ${PARENT_TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_LINKER_FILE:${DEP_TARGET}>"
        "${DIST_DIR}/lib/${PREFIX}/$<TARGET_LINKER_FILE_NAME:${DEP_TARGET}>"
      COMMENT "Copying '${DEP_TARGET}' linker output to output directory")
  endif()
endfunction()

# recursively finds the include paths of dependency engine modules/libraries and saves them to the given variable
function(find_dep_module_includes DEST)
  set(MODULE_MASTER_DEPENDENCY_INCLUDES "")
  foreach(id IN LISTS MODULE_ENGINE_LIB_DEPS MODULE_ENGINE_MOD_DEPS)
    get_property(MODULE_INC_DIR GLOBAL PROPERTY ${id}_INCLUDE_DIRS)
    list(APPEND MODULE_MASTER_DEPENDENCY_INCLUDES "${MODULE_INC_DIR}")
  endforeach()
  set(${DEST} "${MODULE_MASTER_DEPENDENCY_INCLUDES}" PARENT_SCOPE)
endfunction()

# recursively finds the module's top-level include dirs and saves them to the given variable
function(find_module_includes DEST)
  # load this module's include dirs
  set(ALL_INCLUDES "${MODULE_PROJECT_DIR}/${INCLUDE_DIR_NAME};${MODULE_GENERATED_DIR}/${INCLUDE_DIR_NAME}")

  # load include dirs defined by module dependencies
  find_dep_module_includes(PARENT_INCLUDES)
  list(LENGTH PARENT_INCLUDES LEN)
  if(LEN GREATER 0)
    list(APPEND ALL_INCLUDES "${PARENT_INCLUDES}")
  endif()

  # load include dirs defined by this module
  list(LENGTH MODULE_INCLUDES LEN)
  if(LEN GREATER 0)
    list(APPEND ALL_INCLUDES "${MODULE_INCLUDES}")
  endif()

  # remove any duplicates
  list(REMOVE_DUPLICATES ALL_INCLUDES)

  set_property(GLOBAL PROPERTY ${PROJECT_NAME}_INCLUDE_DIRS "${ALL_INCLUDES}")
  set_property(GLOBAL PROPERTY INCLUDE_DIRECTORIES "${ALL_INCLUDES}")

  set(${DEST} "${ALL_INCLUDES}" PARENT_SCOPE)
endfunction()

function(compute_dep_edges)
  get_property(EDGES GLOBAL PROPERTY "DEPENDENCY_GRAPH_EDGES")

  if(NOT "${MODULE_ENGINE_MOD_DEPS}" STREQUAL "")
    foreach(dep ${MODULE_ENGINE_MOD_DEPS})
      list(APPEND EDGES "${dep}/${PROJECT_NAME}")
    endforeach()
  endif()

  set_property(GLOBAL PROPERTY "DEPENDENCY_GRAPH_EDGES" "${EDGES}")
endfunction()

function(top_sort NODES EDGES OUT_LIST)
  # make sure the edges are valid wrt the given node list (i.e. projects aren't declaring bogus dependencies)
  foreach(edge ${EDGES})
    string(REPLACE "/" ";" edge_arr "${edge}")
    list(GET edge_arr 0 SRC_NODE)
    list(GET edge_arr 1 DEST_NODE)
    list(FIND NODES "${SRC_NODE}" SRC_INDEX)
    list(FIND NODES "${DEST_NODE}" DEST_INDEX)

    if(SRC_INDEX EQUAL -1)
      message(FATAL_ERROR "Project \"${SRC_NODE}\" exists in dependency graph but could not be found.")
    endif()

    if(DEST_INDEX EQUAL -1)
      message(FATAL_ERROR "Project \"${SRC_NODE}\" declares dependency on \"${DEST_NODE}\" but no such project was found.")
    endif()
  endforeach()

  set(START_NODES "${NODES}")

  # find the initial list of start nodes (nodes without an incoming edge)
  foreach(edge ${EDGES})
    string(REPLACE "/" ";" edge_arr "${edge}")
    list(GET edge_arr 1 DEST_NODE)
    list(REMOVE_ITEM START_NODES "${DEST_NODE}")
  endforeach()

  list(REMOVE_DUPLICATES START_NODES)

  set(SORTED_NODES "")

  set(CUR_EDGES "${EDGES}")

  # perform a topological sort using Kahn's algorithm
  while(1)
    list(LENGTH START_NODES SN_COUNT)
    if(${SN_COUNT} EQUAL 0)
      break()
    endif()

    list(GET START_NODES 0 CUR_NODE)
    list(REMOVE_AT START_NODES 0)
    list(APPEND SORTED_NODES "${CUR_NODE}")

    set(DEST_IN_EDGES "")
    set(CUR_OUT_EDGES "${CUR_EDGES}")
    list(FILTER CUR_OUT_EDGES INCLUDE REGEX "^${CUR_NODE}/")
    foreach(edge ${CUR_OUT_EDGES})
      string(REPLACE "/" ";" edge_arr "${edge}")
      # get the destination of the edge (the current node is the source)
      list(GET edge_arr 1 DEST_NODE)
      # remove the edge from the graph
      list(REMOVE_ITEM CUR_EDGES "${edge}")
      # find the number of other incoming edges to the destination node
      list(FILTER DEST_IN_EDGES INCLUDE REGEX "/${DEST_NODE}$")
      list(LENGTH DEST_IN_EDGES DEST_IN_EDGE_COUNT)
      # if no edges, add it to the list of start nodes
      if(${DEST_IN_EDGE_COUNT} EQUAL 0)
        list(APPEND START_NODES "${DEST_NODE}")
      endif()
    endforeach()
  endwhile()

  list(LENGTH CUR_EDGES CUR_EDGES_COUNT)
  if(${CUR_EDGES_COUNT} GREATER 0)
    message(FATAL_ERROR "Project dependency graph contains at least one cycle; cannot continue.")
  endif()

  list(REMOVE_DUPLICATES SORTED_NODES)

  set(${OUT_LIST} "${SORTED_NODES}" PARENT_SCOPE)
endfunction()

function(add_header_files BASE_DIR)
  list(APPEND LOCAL_INCLUDE_DIRS ${BASE_DIR})

  set(CONCAT_INCLUDE_DIRS ${INCLUDE_DIRS})

  list(APPEND CONCAT_INCLUDE_DIRS ${LOCAL_INCLUDE_DIRS})

  set(INCLUDE_DIRS ${CONCAT_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

function(add_source_files BASE_DIR)
  file(GLOB_RECURSE LOCAL_C_FILES ${BASE_DIR}/*.c)
  file(GLOB_RECURSE LOCAL_CPP_FILES ${BASE_DIR}/*.cpp)

  set(CONCAT_C_FILES ${C_FILES})
  set(CONCAT_CPP_FILES ${CPP_FILES})

  list(APPEND CONCAT_C_FILES ${LOCAL_C_FILES})
  list(APPEND CONCAT_CPP_FILES ${LOCAL_CPP_FILES})

  set(C_FILES ${CONCAT_C_FILES} PARENT_SCOPE)
  set(CPP_FILES ${CONCAT_CPP_FILES} PARENT_SCOPE)
endfunction()

function(add_source_file FILE_PATH)
  if(FILE_PATH MATCHES "\.c$")
    set(CONCAT_FILES ${C_FILES})
    list(APPEND CONCAT_FILES "${FILE_PATH}")
    set(C_FILES ${CONCAT_FILES} PARENT_SCOPE)
  elseif(FILE_PATH MATCHES "\.cpp$")
    set(CONCAT_FILES ${CPP_FILES})
    list(APPEND CONCAT_FILES "${FILE_PATH}")
    set(CPP_FILES ${CONCAT_FILES} PARENT_SCOPE)
  endif()
endfunction()

function(configure_module MODULE_PROJECT_DIR)
  set(MODULE_PROPS_PATH "${MODULE_PROJECT_DIR}/module.properties")
  if((NOT EXISTS "${MODULE_PROPS_PATH}") OR (IS_DIRECTORY "${MODULE_PROPS_PATH}"))
    message(AUTHOR_WARNING "Failed to find module.properties in requested path ${MODULE_PROJECT_DIR} - skipping")
    return()
  endif()

  file(READ "${MODULE_PROPS_PATH}" MODULE_PROPS_CONTENT)
  string(REPLACE "\r\n" "\n" MODULE_PROPS_CONTENT "${MODULE_PROPS_CONTENT}")
  # next line is a hack to avoid inadvertently escaping the list separator
  string(REPLACE "\\\n" "\\ \n" MODULE_PROPS_CONTENT "${MODULE_PROPS_CONTENT}")
  string(REPLACE ";" "\\;" MODULE_PROPS_CONTENT "${MODULE_PROPS_CONTENT}")
  string(REPLACE "\n" ";" MODULE_PROPS_LINES "${MODULE_PROPS_CONTENT}")
  set(span_lines 0)
  foreach(line ${MODULE_PROPS_LINES})
    string(STRIP "${line}" trimmed)

    if("${trimmed}" STREQUAL "")
      continue()
    endif()

    if("${trimmed}" MATCHES "^[!#]")
      continue()
    endif()

    string(REGEX MATCH "\\\\s*$" trailing_slash "${trimmed}")

    set(value_only "${span_lines}")

    if(trailing_slash)
      set(span_lines 1)
      string(REGEX REPLACE "\\\\s*$" "" trimmed "${trimmed}")
    else()
      set(span_lines 0)
    endif()

    if(${value_only})
      set(cur_value "${cur_value}${trimmed}")
    else()
      string(REGEX MATCH "^(.*[^\\\\])=" cur_key "${trimmed}")
      string(REGEX REPLACE "[ \t\r\n]*=$" "" cur_key "${cur_key}")
      string(STRIP "${cur_key}" cur_key)
      string(REGEX MATCH "[^\\\\]=(.*)$" cur_value "${trimmed}")
      string(REGEX REPLACE "^.=[ \t\r\n]*" "" cur_value "${cur_value}")
      string(STRIP "${cur_value}" cur_value)
    endif()

    if(NOT ${span_lines})
      string(REPLACE "\\\\" "\\" cur_value "${cur_value}") 
      string(REGEX REPLACE "(^|[^\\])\\\\n" "\\1\n" cur_value "${cur_value}")
      string(REGEX REPLACE "(^|[^\\])\\\\r" "\\1\r" cur_value "${cur_value}")
      string(REGEX REPLACE "(^|[^\\])\\\\t" "\\1\t" cur_value "${cur_value}")

      # try to match key
      if("${cur_key}" STREQUAL "${MODULE_PROP_NAME}")
        set(MODULE_NAME "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_TYPE}")
        set(MODULE_TYPE "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LANGS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_LANGUAGES "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_INCLUDE_DIRS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_INCLUDES "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_MOD_DEPS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_ENGINE_MOD_DEPS "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LIB_DEPS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_ENGINE_LIB_DEPS "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LINKER_DEPS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_LINKER_DEPS "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LINKER_DEPS_MSVC}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        if(MSVC)
          list(APPEND MODULE_LINKER_DEPS "${cur_value}")
          string(REPLACE "," ";" cur_value "${cur_value}")
        endif()
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LINKER_DEPS_GCC}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
          string(REPLACE "," ";" cur_value "${cur_value}")
          list(APPEND MODULE_LINKER_DEPS "${cur_value}")
        endif()
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_REQUIRED_PACKAGES}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_REQUIRED_PACKAGES "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_OPTIONAL_PACKAGES}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_OPTIONAL_PACKAGES "${cur_value}")
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_DEFINITIONS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_DEFINITIONS "${cur_value}")
      endif()
    endif()
  endforeach()

  foreach(pack ${MODULE_REQUIRED_PACKAGES})
    find_package("${pack}" REQUIRED)
  endforeach()

  foreach(pack ${MODULE_OPTIONAL_PACKAGES})
    find_package("${pack}")
  endforeach()

  project("${MODULE_NAME}" LANGUAGES ${MODULE_LANGUAGES})

  if("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_LIBRARY}")
    set(MODULE_GENERATED_DIR "${ENGINE_LIBS_GENERATED_DIR}/${MODULE_NAME}")
  elseif("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_STATIC}")
    set(MODULE_GENERATED_DIR "${STATIC_MODULES_GENERATED_DIR}/${MODULE_NAME}")
  elseif("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_DYNAMIC}")
    set(MODULE_GENERATED_DIR "${DYNAMIC_MODULES_GENERATED_DIR}/${MODULE_NAME}")
  endif()

  set(MODULE_CMAKE_PATH "${MODULE_PROJECT_DIR}/module.cmake")
  if(EXISTS "${MODULE_CMAKE_PATH}")
    include("${MODULE_CMAKE_PATH}")
  endif()

  string(CONFIGURE "${MODULE_INCLUDES}" MODULE_INCLUDES)
  string(CONFIGURE "${MODULE_DEFINITIONS}" MODULE_DEFINITIONS)
  string(CONFIGURE "${MODULE_ENGINE_MOD_DEPS}" MODULE_ENGINE_MOD_DEPS)
  string(CONFIGURE "${MODULE_ENGINE_LIB_DEPS}" MODULE_ENGINE_LIB_DEPS)
  string(CONFIGURE "${MODULE_LINKER_DEPS}" MODULE_LINKER_DEPS)

  if("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_STATIC}")
    compute_dep_edges()
    set(MODULE_${MODULE_NAME}_DEPS ${MODULE_ENGINE_MOD_DEPS} CACHE STRING "")
  endif()

  set(res_dir "${MODULE_PROJECT_DIR}/res")
  set(res_pack_target "pack_resources_${PROJECT_NAME}")
  set(res_gen_target "gen_resources_${PROJECT_NAME}")
  if(EXISTS "${res_dir}")
    set(arp_out_dir "${CMAKE_BINARY_DIR}/res/arp")
    set(arp_out_name "resources_${PROJECT_NAME}")
    set(arp_out_path "${arp_out_dir}/${arp_out_name}.arp")
    set(supp_mappings_path "${ROOT_PROJECT_DIR}/res/arp_custom_mappings.csv")

    file(MAKE_DIRECTORY "${arp_out_dir}")

    file(GLOB_RECURSE res_files "${res_dir}/*")
    add_custom_command(OUTPUT "${arp_out_path}"
                       COMMAND "${arptool_exe}" "pack" "${res_dir}"
                               "-n" "argus"
                               "-o" "${arp_out_dir}"
                               "-f" "${arp_out_name}"
                               "-c" "deflate"
                               "-m" "${supp_mappings_path}"
                       DEPENDS "${res_files}")

    add_custom_target("${res_pack_target}" DEPENDS "arptool" "${arp_out_path}")

    set(abacus_out_dir "${CMAKE_BINARY_DIR}/res/abacus")
    set(h_out_dir_base "${MODULE_GENERATED_DIR}/${INCLUDE_DIR_NAME}")
    set(h_out_dir "${h_out_dir_base}/internal/${PROJECT_NAME}")
    set(c_out_dir_base "${MODULE_GENERATED_DIR}/${SOURCE_DIR_NAME}")
    set(c_out_dir "${c_out_dir_base}/res")

    file(MAKE_DIRECTORY "${h_out_dir}")
    file(MAKE_DIRECTORY "${c_out_dir}")

    set(arp_file_name "${arp_out_name}.arp")
    set(h_out_path "${h_out_dir}/resources.h")
    set(c_out_path "${c_out_dir}/${arp_out_name}.arp.c")
    add_custom_command(OUTPUT "${c_out_path}"
                       COMMAND "ruby" "${CMAKE_SOURCE_DIR}/external/tooling/abacus/abacus.rb"
                               "-i" "${arp_out_path}"
                               "-n" "${arp_file_name}"
                               "-s" "${c_out_path}"
                       DEPENDS "${res_pack_target}" "${arp_out_path}")

    execute_process(COMMAND "ruby" "${CMAKE_SOURCE_DIR}/external/tooling/abacus/abacus.rb"
                            "-n" "${arp_out_name}.arp"
                            "-h" "${h_out_path}"
                    RESULT_VARIABLE CMD_RES)
    if(CMD_RES)
      message(FATAL_ERROR "    abacus.rb: ${CMD_RES}")
    endif()

    add_custom_target("${res_gen_target}" DEPENDS "${res_pack_target}" "${h_out_path}" "${c_out_path}")

    add_source_file("${c_out_path}")
  endif()

  # basic source files
  add_header_files("${MODULE_PROJECT_DIR}/${INCLUDE_DIR_NAME}")
  add_source_files("${MODULE_PROJECT_DIR}/${SOURCE_DIR_NAME}")

  # generated source files
  add_header_files("${MODULE_GENERATED_DIR}/${INCLUDE_DIR_NAME}")
  add_source_files("${MODULE_GENERATED_DIR}/${SOURCE_DIR_NAME}")

  list(LENGTH LOCAL_SRC_PATHS LEN)
  if(LEN GREATER 0)
    foreach(path ${LOCAL_SRC_PATHS})
      add_source_files("${path}")
    endforeach()
  endif()

  if(NOT "${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_DYNAMIC}")
    get_property(COMBINED_TARGET_INCLUDES GLOBAL PROPERTY COMBINED_TARGET_INCLUDES)
    list(APPEND COMBINED_TARGET_INCLUDES "${MODULE_INCLUDES}")
    set_property(GLOBAL PROPERTY COMBINED_TARGET_INCLUDES "${COMBINED_TARGET_INCLUDES}")

    # only include libraries if they're explicitly requested from a static module
    if("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_STATIC}")
      set(STATIC_MODULE_LIBS_LOCAL "$<TARGET_OBJECTS:${id}>;${STATIC_MODULE_LIBS}")
      foreach(lib ${MODULE_ENGINE_LIB_DEPS})
        set(STATIC_MODULE_LIBS_LOCAL "$<TARGET_OBJECTS:${lib}>;${STATIC_MODULE_LIBS_LOCAL}")
      endforeach()
      set(STATIC_MODULE_LIBS "${STATIC_MODULE_LIBS_LOCAL}" PARENT_SCOPE)
    endif()

    # configure the copy headers task to include this project
    set(MODULE_INCLUDE_DIR "${MODULE_PROJECT_DIR}/${INCLUDE_DIR_NAME}/argus")
    add_custom_command(TARGET ${HDR_TARGET} POST_BUILD
      COMMENT "Copying headers for module ${id}"
      COMMAND ${CMAKE_COMMAND} -E
        copy_directory ${MODULE_INCLUDE_DIR} ${HDR_OUT_DIR})

    # add this project as a dependency of the copy headers task
    add_dependencies(${HDR_TARGET} ${id})
  endif()

  if("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_DYNAMIC}")
  # compile in libraries if any are requested if they're not in the base library
    set(DYN_LIBS "")
    foreach(lib ${MODULE_ENGINE_LIB_DEPS})
      set(gen_expr "$<TARGET_OBJECTS:${lib}>")
      list(FIND STATIC_MODULE_LIBS "${gen_expr}" found_index)
      if(found_index EQUAL -1)
        set(DYN_LIBS "${gen_expr};${DYN_LIBS}")
      endif()
    endforeach()

    add_library(${PROJECT_NAME} MODULE ${C_FILES} ${CPP_FILES} ${DYN_LIBS})

    # enable PIC
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    if(NOT MSVC)
      target_compile_options("${PROJECT_NAME}" PRIVATE "-fvisibility=hidden")
    endif()

    # output to separate directory
    set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${DYN_MODULE_DIR}")

    set(PROJECT_LINKER_DEPS ${MODULE_LINKER_DEPS})
    list(APPEND PROJECT_LINKER_DEPS ${LIB_BASE_NAME})
    target_link_libraries(${PROJECT_NAME} ${PROJECT_LINKER_DEPS})

    copy_dep_output("${PROJECT_NAME}" "${PROJECT_NAME}" "${DYN_MODULE_DIR}")
  else()
    add_library(${PROJECT_NAME} OBJECT ${C_FILES} ${CPP_FILES})

    get_property(COMBINED_TARGET_LINKER_DEPS GLOBAL PROPERTY COMBINED_TARGET_LINKER_DEPS)
    list(APPEND COMBINED_TARGET_LINKER_DEPS "${MODULE_LINKER_DEPS}")
    set_property(GLOBAL PROPERTY COMBINED_TARGET_LINKER_DEPS "${COMBINED_TARGET_LINKER_DEPS}")
  endif()

  target_compile_definitions("${PROJECT_NAME}" PUBLIC "${MODULE_DEFINITIONS}")

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions("${PROJECT_NAME}" PUBLIC "_ARGUS_DEBUG_MODE")
  endif()

  if(WIN32)
    target_compile_definitions("${PROJECT_NAME}" PUBLIC "GLFW_DLL")
    target_compile_definitions("${PROJECT_NAME}" PUBLIC "NOMINMAX")
  endif()

  # recursively load this module's include dirs
  find_module_includes(PROJECT_INCLUDES)
  list(APPEND INCLUDE_DIRS "${PROJECT_INCLUDES}")
  target_include_directories("${PROJECT_NAME}" PUBLIC "${INCLUDE_DIRS}")

  if(TARGET ${res_gen_target})
    add_dependencies("${PROJECT_NAME}" "${res_gen_target}")
  endif()

  # set the C++ standard
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD "${PROJECT_CXX_VERSION}")
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS "${PROJECT_CXX_EXTENSIONS}")
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)

  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

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
  configure_module("${ENGINE_LIBS_SRC_DIR}/${id}")
endforeach()

foreach(id ${STATIC_MODULE_IDS})
  configure_module("${STATIC_MODULES_SRC_DIR}/${id}")
endforeach()

# IMPORTANT: these should be configured last so that they can avoid compiling in
# libraries already present in the base library
foreach(id ${DYNAMIC_MODULE_IDS})
  configure_module("${DYNAMIC_MODULES_SRC_DIR}/${id}")
endforeach()

# configure module dependency information

get_property(DEP_EDGES GLOBAL PROPERTY "DEPENDENCY_GRAPH_EDGES")
top_sort("${STATIC_MODULE_IDS}" "${DEP_EDGES}" STATIC_MODULE_IDS)

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
add_library(${LIB_BASE_NAME} SHARED ${STATIC_MODULE_LIBS})

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
target_link_libraries(${LIB_BASE_NAME} ${COMBINED_TARGET_LINKER_DEPS})

# add global include list
get_property(COMBINED_TARGET_INCLUDES GLOBAL PROPERTY COMBINED_TARGET_INCLUDES)
list(REMOVE_DUPLICATES COMBINED_TARGET_INCLUDES)
target_include_directories(${LIB_BASE_NAME} PUBLIC "${COMBINED_TARGET_INCLUDES}")

foreach(module ${DYNAMIC_MODULE_IDS})
  add_dependencies("${module}" "${LIB_BASE_NAME}")
endforeach()

# make the copy headers target a dependency to force it to run
add_dependencies(${LIB_BASE_NAME} ${HDR_TARGET})

copy_dep_output("${LIB_BASE_NAME}" "${LIB_BASE_NAME}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${ZLIB_LIBRARY}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${PNG_LIBRARY}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${GLFW_LIBRARY_BASE}" ".")
copy_dep_output("${LIB_BASE_NAME}" "${ARP_LIBRARY_BASE}" ".")
