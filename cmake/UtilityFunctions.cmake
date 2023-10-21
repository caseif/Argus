function(add_file_action)
  # Parse arguments
  set(sv_args TARGET ACTION SOURCE DEST DEPENDS GENERATED_TARGET)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${sv_args}" "")

  if(NOT ARG_TARGET OR NOT ARG_ACTION OR NOT ARG_SOURCE OR NOT ARG_DEST)
    message(FATAL_ERROR "Missing required arguments")
  endif()

  set(TIMESTAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/timestamps)

  string(MD5 DEST_HASH "${ARG_DEST}")

  get_filename_component(DEST_DIR "${ARG_DEST}" DIRECTORY)

  set(TIMESTAMP_FILE "${TIMESTAMP_DIR}/${DEST_HASH}.timestamp")

  set(BUILT_COMMAND "${CMAKE_COMMAND}" "-E" "${ARG_ACTION}" "${ARG_SOURCE}" "${ARG_DEST}")

  set(DEPENDS_LIST "${ARG_TARGET}")
  if(ARG_DEPENDS)
    list(APPEND DEPENDS_LIST "${ARG_DEPENDS}")
  endif()
  if(TARGET "${ARG_SOURCE}")
    list(APPEND DEPENDS_LIST "${ARG_SOURCE}")
  endif()

  add_custom_command(OUTPUT "${TIMESTAMP_FILE}"
      DEPENDS ${DEPENDS_LIST}
      COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${DEST_DIR}"
      COMMAND ${BUILT_COMMAND}
      COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${TIMESTAMP_DIR}"
      COMMAND "${CMAKE_COMMAND}" "-E" "touch" "${TIMESTAMP_FILE}")

  set(GEN_TARGET_NAME "${ARG_ACTION}-${DEST_HASH}")
  add_custom_target("${GEN_TARGET_NAME}" ALL
      DEPENDS ${TIMESTAMP_FILE})

  if(GENERATED_TARGET)
    set("${GENERATED_TARGET}" "${GEN_TARGET_NAME}")
  endif()
endfunction()

function(create_so_symlinks TARGET LIB_FILE_DIR)
  if(WIN32)
    return()
  endif()

  get_target_property(TARGET_SOVERSION "${TARGET}" SOVERSION)

  if(TARGET_SOVERSION STREQUAL "TARGET_SOVERSION-NOTFOUND")
    return()
  endif()

  get_target_property(TARGET_DEBUG_POSTFIX "${TARGET}" DEBUG_POSTFIX)
  if(TARGET_DEBUG_POSTFIX STREQUAL "TARGET_DEBUG_POSTFIX-NOTFOUND")
    set(TARGET_DEBUG_POSTFIX "")
  endif()

  get_target_property(TARGET_PREFIX "${TARGET}" PREFIX)
  if(TARGET_PREFIX STREQUAL "TARGET_PREFIX-NOTFOUND")
    set(TARGET_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")
  endif()

  get_target_property(TARGET_SUFFIX "${TARGET}" SUFFIX)
  if(TARGET_SUFFIX STREQUAL "TARGET_SUFFIX-NOTFOUND")
    set(TARGET_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
  endif()

  get_target_property(TARGET_OUT_NAME "${TARGET}" LIBRARY_OUTPUT_NAME_${CMAKE_BUILD_TYPE})
  if(TARGET_OUT_NAME MATCHES "TARGET_OUT_NAME-NOTFOUND")
    get_target_property(TARGET_OUT_NAME "${TARGET}" LIBRARY_OUTPUT_NAME)
    if(TARGET_OUT_NAME MATCHES "TARGET_OUT_NAME-NOTFOUND")
      get_target_property(TARGET_OUT_NAME "${TARGET}" OUTPUT_NAME_${CMAKE_BUILD_TYPE})
      if(TARGET_OUT_NAME MATCHES "TARGET_OUT_NAME-NOTFOUND")
        get_target_property(TARGET_OUT_NAME "${TARGET}" OUTPUT_NAME)
        if(TARGET_OUT_NAME MATCHES "TARGET_OUT_NAME-NOTFOUND")
          set(TARGET_OUT_NAME "${TARGET}")
        endif()
      endif()
    endif()
  endif()

  string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower)
  if(build_type_lower MATCHES "debug")
    set(SO_POSTFIX "${TARGET_DEBUG_POSTFIX}")
  else()
    set(SO_POSTFIX "")
  endif()

  if(APPLE)
    set(LINK_MAJOR_NAME "${TARGET_PREFIX}${TARGET_OUT_NAME}${SO_POSTFIX}.${TARGET_SOVERSION}${TARGET_SUFFIX}")
  else()
    set(LINK_MAJOR_NAME "${TARGET_PREFIX}${TARGET_OUT_NAME}${SO_POSTFIX}${TARGET_SUFFIX}.${TARGET_SOVERSION}")
  endif()

  set(LINK_BASE_NAME "${TARGET_PREFIX}${TARGET_OUT_NAME}${SO_POSTFIX}${TARGET_SUFFIX}")

  add_file_action(TARGET "${TARGET}"
      ACTION "create_symlink"
      SOURCE "$<TARGET_FILE:${TARGET}>"
      DEST "${LIB_FILE_DIR}/${LINK_MAJOR_NAME}"
      GENERATED_TARGET SONAME_TARGET)

  add_file_action(TARGET "${TARGET}"
      ACTION "create_symlink"
      SOURCE "${LIB_FILE_DIR}/${LINK_MAJOR_NAME}"
      DEST "${LIB_FILE_DIR}/${LINK_BASE_NAME}")
endfunction()

# init task to copy dependency output
function(_argus_copy_dep_output DIST_DIR TARGET PREFIX)
  if(PREFIX MATCHES "^$")
    set(PREFIX ".")
  endif()

  set(DEST_DIR "${DIST_DIR}/${PREFIX}")
  set(LIB_FILE_DEST_PATH "${DEST_DIR}/$<TARGET_FILE_NAME:${TARGET}>")

  add_file_action(TARGET "${TARGET}"
      ACTION "copy_if_different"
      SOURCE "$<TARGET_FILE:${TARGET}>"
      DEST "${LIB_FILE_DEST_PATH}")

  get_target_property(TARGET_TYPE "${TARGET}" "TYPE")

  if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
    create_so_symlinks("${TARGET}" "${DEST_DIR}")
  endif()

  if(WIN32 AND "${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
    set(LINKER_FILE_DEST_PATH "${DIST_DIR}/lib/${PREFIX}/$<TARGET_LINKER_FILE_NAME:${TARGET}>")

    add_file_action(TARGET "${TARGET}"
        ACTION "copy_if_different"
        SOURCE "$<TARGET_LINKER_FILE:${TARGET}>"
        DEST "${LINKER_FILE_DEST_PATH}")
  endif()
endfunction()

# recursively finds the include paths of dependency engine modules/libraries and saves them to the given variable
function(_argus_find_dep_module_includes DEST LIB_DEPS MOD_DEPS)
  set(MODULE_MASTER_DEPENDENCY_INCLUDES "")
  foreach(id IN LISTS LIB_DEPS MOD_DEPS)
    get_property(MODULE_INC_DIR GLOBAL PROPERTY ${id}_INCLUDE_DIRS)
    list(APPEND MODULE_MASTER_DEPENDENCY_INCLUDES "${MODULE_INC_DIR}")
  endforeach()
  set(${DEST} "${MODULE_MASTER_DEPENDENCY_INCLUDES}" PARENT_SCOPE)
endfunction()

function(_argus_set_module_includes MODULE_ID INCLUDES)
  set_property(GLOBAL PROPERTY ${MODULE_ID}_INCLUDE_DIRS "${INCLUDES}")
endfunction()

# recursively finds the module's top-level include dirs and saves them to the given variable
function(_argus_find_module_includes DEST PROJECT_DIR GENERATED_DIR LIB_DEPS MOD_DEPS)
  # load this module's include dirs
  set(ALL_INCLUDES "${PROJECT_DIR}/${INCLUDE_DIR_NAME};${GENERATED_DIR}/${INCLUDE_DIR_NAME}")

  # load include dirs defined by module dependencies
  _argus_find_dep_module_includes(PARENT_INCLUDES "${LIB_DEPS}" "${MOD_DEPS}")
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

  _argus_set_module_includes("${PROJECT_NAME}" "${ALL_INCLUDES}")

  set(${DEST} "${ALL_INCLUDES}" PARENT_SCOPE)
endfunction()

function(_argus_compute_dep_edges)
  get_property(EDGES GLOBAL PROPERTY "DEPENDENCY_GRAPH_EDGES")

  if(NOT "${MODULE_ENGINE_MOD_DEPS}" STREQUAL "")
    foreach(dep ${MODULE_ENGINE_MOD_DEPS})
      list(APPEND EDGES "${dep}/${PROJECT_NAME}")
    endforeach()
  endif()

  set_property(GLOBAL PROPERTY "DEPENDENCY_GRAPH_EDGES" "${EDGES}")
endfunction()

function(_argus_topo_sort NODES EDGES OUT_LIST)
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

  # nodes which have no incoming edges
  set(START_NODES "")
  # nodes which have already been checked for incoming edges
  set(SEEN_NODES "")

  # compute list of start nodes
  foreach(edge ${EDGES})
    string(REPLACE "/" ";" edge_arr "${edge}")
    list(GET edge_arr 0 node)

    # skip nodes that have already been considered
    list(FIND SEEN_NODES "${node}" seen_node)
    if(NOT ${seen_node} EQUAL -1)
      continue()
    endif()
    list(APPEND SEEN_NODES "${node}")

    # find all incoming edges for this node
    set(IN_EDGES "${EDGES}")
    list(FILTER IN_EDGES INCLUDE REGEX "\\/${node}")
    list(LENGTH IN_EDGES IN_EDGE_COUNT)
    # if no incoming edges, add it to the list of start nodes
    if(${IN_EDGE_COUNT} EQUAL 0)
      list(APPEND START_NODES "${node}")
    endif()
  endforeach()

  # final list of sorted nodes
  set(SORTED_NODES "")

  # mutable copy of all edges; edges will be removed as they're processed
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

    # get all outgoing edges from current node
    set(CUR_OUT_EDGES "${CUR_EDGES}")
    list(FILTER CUR_OUT_EDGES INCLUDE REGEX "^${CUR_NODE}/")

    foreach(edge ${CUR_OUT_EDGES})
      # get the destination of the edge
      string(REPLACE "/" ";" edge_arr "${edge}")
      list(GET edge_arr 1 DEST_NODE)
      # remove the edge from the graph
      list(REMOVE_ITEM CUR_EDGES "${edge}")
      # find incoming edges to the destination node
      set(DEST_IN_EDGES "${CUR_EDGES}")
      list(FILTER DEST_IN_EDGES INCLUDE REGEX "\\/${DEST_NODE}$")
      # get the number of other incoming edges to the destination node
      list(LENGTH DEST_IN_EDGES DEST_IN_EDGE_COUNT)
      # if no other incoming edges, add it to the list of start nodes
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

function(_argus_add_header_files OUT_INCLUDE_DIRS BASE_DIR)
  list(APPEND LOCAL_INCLUDE_DIRS ${BASE_DIR})

  set(CONCAT_INCLUDE_DIRS ${INCLUDE_DIRS})

  list(APPEND CONCAT_INCLUDE_DIRS ${LOCAL_INCLUDE_DIRS})

  set(${OUT_INCLUDE_DIRS} ${CONCAT_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

function(_argus_add_source_files OUT_C_FILES OUT_CPP_FILES BASE_DIR)
  file(GLOB_RECURSE LOCAL_C_FILES ${BASE_DIR}/*.c)
  file(GLOB_RECURSE LOCAL_CPP_FILES ${BASE_DIR}/*.cpp)

  set(CONCAT_C_FILES ${C_FILES})
  set(CONCAT_CPP_FILES ${CPP_FILES})

  list(APPEND CONCAT_C_FILES ${LOCAL_C_FILES})
  list(APPEND CONCAT_CPP_FILES ${LOCAL_CPP_FILES})

  set(${OUT_C_FILES} ${CONCAT_C_FILES} PARENT_SCOPE)
  set(${OUT_CPP_FILES} ${CONCAT_CPP_FILES} PARENT_SCOPE)
endfunction()

function(_argus_add_source_file OUT_C_FILES OUT_CPP_FILES FILE_PATH)
  if(FILE_PATH MATCHES "\.c$")
    set(CONCAT_FILES ${C_FILES})
    list(APPEND CONCAT_FILES "${FILE_PATH}")
    set(${OUT_C_FILES} ${CONCAT_FILES} PARENT_SCOPE)
  elseif(FILE_PATH MATCHES "\.cpp$")
    set(CONCAT_FILES ${CPP_FILES})
    list(APPEND CONCAT_FILES "${FILE_PATH}")
    set(${OUT_CPP_FILES} ${CONCAT_FILES} PARENT_SCOPE)
  endif()
endfunction()

function(_argus_append_source_files TARGET DIR)
  file(GLOB_RECURSE supp_c_files "${DIR}/*.c")
  file(GLOB_RECURSE supp_cpp_files "${DIR}/*.cpp")
  foreach(file "${supp_c_files}")
    target_sources("${TARGET}" PRIVATE "${file}")
  endforeach()
  foreach(file "${supp_cpp_files}")
    target_sources("${TARGET}" PRIVATE "${file}")
  endforeach()
endfunction()

function(_argus_disable_warnings targets)
  foreach(target ${targets})
    if(NOT TARGET "${target}")
      message(WARNING "Target '${target}' does not exist")
      return()
    endif()

    get_target_property(TARGET_CXX_FLAGS "${target}" COMPILE_OPTIONS)

    if(TARGET_CXX_FLAGS)
      # remove any present warning flags
      if(MSVC)
        list(FILTER TARGET_CXX_FLAGS EXCLUDE REGEX "^(/W[0-4])|(\\$<\\$<(.*)>:/W[0-4]>)$")
      elseif(GCC OR CLANG)
        list(FILTER TARGET_CXX_FLAGS EXCLUDE REGEX "^(-W)|(\\$<\\$<(.*)>:-W(.*)>$)")
      else()
        message(WARNING "Unknown compiler, unable to disable warnings for dependencies")
        return()
      endif()

      # assign the modified flags back to the target
      set_target_properties("${target}" PROPERTIES COMPILE_OPTIONS "${TARGET_CXX_FLAGS}")
    endif()

    # add flag to disable all warnings
    if(MSVC)
      target_compile_options(${target} PRIVATE "/W0")
    elseif(GCC OR CLANG)
      target_compile_options(${target} PRIVATE "-w")
    endif()

    get_target_property(TARGET_CXX_FLAGS "${target}" COMPILE_OPTIONS)
  endforeach()
endfunction()
