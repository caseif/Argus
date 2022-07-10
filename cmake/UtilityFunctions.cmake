# init task to copy dependency output
function(_argus_copy_dep_output DIST_DIR PARENT_TARGET DEP_TARGET PREFIX)
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
