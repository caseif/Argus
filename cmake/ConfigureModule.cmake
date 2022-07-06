include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ConfigureCompileFlags.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/UtilityFunctions.cmake")

function(_argus_configure_module MODULE_PROJECT_DIR ROOT_DIR CXX_STANDARD CXX_EXTENSIONS ARPTOOL_EXE_PATH)
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
    _argus_compute_dep_edges()
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
                       COMMAND "${ARPTOOL_EXE_PATH}" "pack" "${res_dir}"
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

    _argus_add_source_file(C_FILES CPP_FILES "${c_out_path}")
  endif()

  # basic source files
  _argus_add_header_files(INCLUDE_DIRS "${MODULE_PROJECT_DIR}/${INCLUDE_DIR_NAME}")
  _argus_add_source_files(C_FILES CPP_FILES "${MODULE_PROJECT_DIR}/${SOURCE_DIR_NAME}")

  # generated source files
  _argus_add_header_files(INCLUDE_DIRS "${MODULE_GENERATED_DIR}/${INCLUDE_DIR_NAME}")
  _argus_add_source_files(C_FILES CPP_FILES "${MODULE_GENERATED_DIR}/${SOURCE_DIR_NAME}")

  list(LENGTH LOCAL_SRC_PATHS LEN)
  if(LEN GREATER 0)
    foreach(path ${LOCAL_SRC_PATHS})
      _argus_add_source_files(C_FILES CPP_FILES "${path}")
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

    _argus_set_compile_flags(${PROJECT_NAME})

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

    _argus_copy_dep_output("${DIST_DIR}" "${PROJECT_NAME}" "${PROJECT_NAME}" "${DYN_MODULE_DIR}")
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
  _argus_find_module_includes(PROJECT_INCLUDES)
  list(APPEND INCLUDE_DIRS "${PROJECT_INCLUDES}")
  target_include_directories("${PROJECT_NAME}" PUBLIC "${INCLUDE_DIRS}")

  if(TARGET ${res_gen_target})
    add_dependencies("${PROJECT_NAME}" "${res_gen_target}")
  endif()

  # set the C++ standard
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD "${CXX_STANDARD}")
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS "${CXX_EXTENSIONS}")
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)

  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()