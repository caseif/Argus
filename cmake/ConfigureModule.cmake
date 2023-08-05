include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ConfigureCompileFlags.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/UtilityFunctions.cmake")

function(_argus_configure_module MODULE_PROJECT_DIR ROOT_DIR CXX_STANDARD CXX_EXTENSIONS ARPTOOL_EXE_PATH)
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
  set(MODULE_PROP_LINKER_DEPS_WIN32 "linker_deps_win32")
  set(MODULE_PROP_LINKER_DEPS_LINUX "linker_deps_linux")
  set(MODULE_PROP_LINKER_DEPS_MACOS "linker_deps_macos")
  set(MODULE_PROP_EXTERNAL_LINKER_DEPS "external_linker_static_deps")
  set(MODULE_PROP_REQUIRED_PACKAGES "required_packages")
  set(MODULE_PROP_OPTIONAL_PACKAGES "optional_packages")
  set(MODULE_PROP_DEFINITIONS "definitions")

  set(LANG_RUST "RUST")

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
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LINKER_DEPS_WIN32}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        if(WIN32)
          list(APPEND MODULE_LINKER_DEPS "${cur_value}")
          string(REPLACE "," ";" cur_value "${cur_value}")
        endif()
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LINKER_DEPS_LINUX}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        if(UNIX AND NOT APPLE)
          string(REPLACE "," ";" cur_value "${cur_value}")
          list(APPEND MODULE_LINKER_DEPS "${cur_value}")
        endif()
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_LINKER_DEPS_MACOS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        if(APPLE)
          string(REPLACE "," ";" cur_value "${cur_value}")
          list(APPEND MODULE_LINKER_DEPS "${cur_value}")
        endif()
      elseif("${cur_key}" STREQUAL "${MODULE_PROP_EXTERNAL_LINKER_DEPS}")
        string(REPLACE "," ";" cur_value "${cur_value}")
        list(APPEND MODULE_EXTERNAL_LINKER_DEPS "${cur_value}")
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

  if("${MODULE_LANGUAGES}" STREQUAL "${LANG_RUST}")
    set(IS_RUST YES)
    set(IS_EXTERNAL YES)
  else()
    set(IS_EXTERNAL NO)
  endif()

  if (${IS_EXTERNAL})
    project("${MODULE_NAME}")
  else()
    project("${MODULE_NAME}" LANGUAGES ${MODULE_LANGUAGES})
  endif()

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
  string(CONFIGURE "${MODULE_EXTERNAL_LINKER_DEPS}" MODULE_EXTERNAL_LINKER_DEPS)

  if("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_STATIC}")
    _argus_compute_dep_edges()
    set(MODULE_${MODULE_NAME}_DEPS ${MODULE_ENGINE_MOD_DEPS} CACHE STRING "")
  endif()

  # generated sources are not supported for external projects at the moment
  if(NOT ${IS_EXTERNAL})
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
  endif()

  # basic source files
  _argus_add_header_files(INCLUDE_DIRS "${MODULE_PROJECT_DIR}/${INCLUDE_DIR_NAME}")
  if(NOT ${IS_EXTERNAL})
    _argus_add_source_files(C_FILES CPP_FILES "${MODULE_PROJECT_DIR}/${SOURCE_DIR_NAME}")
  endif()

  if(NOT ${IS_EXTERNAL})
    # generated source files
    _argus_add_header_files(INCLUDE_DIRS "${MODULE_GENERATED_DIR}/${INCLUDE_DIR_NAME}")
    _argus_add_source_files(C_FILES CPP_FILES "${MODULE_GENERATED_DIR}/${SOURCE_DIR_NAME}")

    list(LENGTH LOCAL_SRC_PATHS LEN)
    if(LEN GREATER 0)
      foreach(path ${LOCAL_SRC_PATHS})
        _argus_add_source_files(C_FILES CPP_FILES "${path}")
      endforeach()
    endif()
  endif()

  if(NOT "${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_DYNAMIC}")
    get_property(COMBINED_TARGET_INCLUDES GLOBAL PROPERTY COMBINED_TARGET_INCLUDES)
    list(APPEND COMBINED_TARGET_INCLUDES "${MODULE_INCLUDES}")
    set_property(GLOBAL PROPERTY COMBINED_TARGET_INCLUDES "${COMBINED_TARGET_INCLUDES}")

    # only include libraries if they're explicitly requested from a static module
    if("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_STATIC}")
      set(STATIC_MODULE_LIBS_LOCAL "${id};${STATIC_MODULE_LIBS}")
      foreach(lib ${MODULE_ENGINE_LIB_DEPS})
        set(STATIC_MODULE_LIBS_LOCAL "${lib};${STATIC_MODULE_LIBS_LOCAL}")
      endforeach()
      set(STATIC_MODULE_LIBS "${STATIC_MODULE_LIBS_LOCAL}" PARENT_SCOPE)
    endif()

    # configure the copy headers task to include this project
    set(MODULE_INCLUDE_DIR "${MODULE_PROJECT_DIR}/${INCLUDE_DIR_NAME}/argus")
    if(EXISTS "${MODULE_INCLUDE_DIR}")
      add_custom_command(TARGET ${HDR_TARGET} POST_BUILD
        COMMENT "Copying headers for module ${id}"
        COMMAND ${CMAKE_COMMAND} -E
          copy_directory ${MODULE_INCLUDE_DIR} ${HDR_OUT_DIR})
    endif()

    # add this project as a dependency of the copy headers task
    add_dependencies(${HDR_TARGET} ${id})
  endif()

  if("${MODULE_TYPE}" STREQUAL "${PROJECT_TYPE_DYNAMIC}")
    if(${IS_EXTERNAL})
      message(FATAL_ERROR "External build systems are not supported for dynamic modules")
    endif()

    # compile in libraries if any are requested if they're not in the base library
    set(DYN_MODULE_STATIC_LIBS "")
    set(RUST_LIBS_FOR_LINKING "")
    foreach(lib ${MODULE_ENGINE_LIB_DEPS})
      list(FIND STATIC_MODULE_LIBS "${lib}" found_index)
      if(found_index EQUAL -1)
        get_property(lib_is_external TARGET "${lib}" PROPERTY IS_EXTERNAL)
        if(${lib_is_external})
          get_property(lib_is_rust TARGET "${lib}" PROPERTY IS_RUST)

          if(${lib_is_rust})
            list(APPEND RUST_LIBS_FOR_LINKING "${lib}")
          endif()
        else()
          list(APPEND DYN_MODULE_STATIC_LIBS "$<TARGET_OBJECTS:${lib}>")
        endif()
      endif()
    endforeach()

    add_library(${PROJECT_NAME} MODULE ${C_FILES} ${CPP_FILES} ${DYN_MODULE_STATIC_LIBS})

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

    foreach(lib ${RUST_LIBS_FOR_LINKING})
      target_link_libraries(${PROJECT_NAME}
                            debug "${RUST_TARGET_DIR}/${lib}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}"
                            optimized "${RUST_TARGET_DIR}/${lib}/release/${CMAKE_STATIC_LIBRARY_PREFIX}${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endforeach()

    _argus_copy_dep_output("${DIST_DIR}" "${PROJECT_NAME}" "${PROJECT_NAME}" "${DYN_MODULE_DIR}")
  else()
    if(${IS_EXTERNAL})
      if(${IS_RUST})
        ExternalProject_Add(
          "${MODULE_NAME}"
          DOWNLOAD_COMMAND ""
          CONFIGURE_COMMAND ""
          BUILD_COMMAND "${CMAKE_COMMAND}"
                "-E" "env"
                "CARGO_TARGET_DIR=${RUST_TARGET_DIR}/${MODULE_NAME}"
                "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}"
                "cargo" "build" "-vv"
                "--manifest-path=${MODULE_PROJECT_DIR}/Cargo.toml" "$<$<CONFIG:Release>:--release>"
          BUILD_BYPRODUCTS "${RUST_TARGET_DIR}/${MODULE_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/${CMAKE_STATIC_LIBRARY_PREFIX}${MODULE_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
          INSTALL_COMMAND ""
          BUILD_ALWAYS ON
          LOG_BUILD ON
          LOG_OUTPUT_ON_FAILURE ON)
      endif()
    else()
      add_library(${PROJECT_NAME} OBJECT ${C_FILES} ${CPP_FILES})

      _argus_set_compile_flags(${PROJECT_NAME})
    endif()

    set_property(TARGET ${PROJECT_NAME} PROPERTY IS_EXTERNAL ${IS_EXTERNAL})
    set_property(TARGET ${PROJECT_NAME} PROPERTY IS_RUST ${IS_RUST})

    if(${IS_EXTERNAL})
      set(copy_libs_target "${MODULE_NAME}_copy_static_libs")
      add_custom_target(${copy_libs_target})
      add_dependencies(${PROJECT_NAME} ${MODULE_EXTERNAL_LINKER_DEPS})

      set(dest_dir "${CMAKE_BINARY_DIR}/external_projects/${MODULE_NAME}/libs")
      add_custom_command(TARGET ${copy_libs_target} POST_BUILD
        COMMENT "Cleaning static libs directory for module ${MODULE_NAME}"
        COMMAND ${CMAKE_COMMAND} -E
          rm "-r" "-f" "${dest_dir}")
      foreach(dep ${MODULE_EXTERNAL_LINKER_DEPS})
        if("${dep}" STREQUAL "")
          continue()
        endif()
        set(src_file "$<TARGET_FILE:${dep}>")
        if(MSVC)
          # strip debug and version suffixes from file name so Rust doesn't complain
          set(dest_file "${dest_dir}/$<TARGET_LINKER_FILE_PREFIX:${dep}>${dep}$<TARGET_LINKER_FILE_SUFFIX:${dep}>")
        else()
          set(dest_file "${dest_dir}/$<TARGET_LINKER_FILE_NAME:${dep}>")
        endif()
        add_custom_command(TARGET ${copy_libs_target} POST_BUILD
          COMMENT "Copying static library ${dep} for module ${MODULE_NAME}"
          COMMAND ${CMAKE_COMMAND} -E
            copy "${src_file}" "${dest_file}")
      endforeach()

      add_dependencies(${MODULE_NAME} ${copy_libs_target})
    #else()
    endif()
    get_property(COMBINED_TARGET_LINKER_DEPS GLOBAL PROPERTY COMBINED_TARGET_LINKER_DEPS)
    list(APPEND COMBINED_TARGET_LINKER_DEPS "${MODULE_LINKER_DEPS}")
    set_property(GLOBAL PROPERTY COMBINED_TARGET_LINKER_DEPS "${COMBINED_TARGET_LINKER_DEPS}")
  endif()

  # todo: work out how to pass compile definitions to Rust projects
  if(${IS_EXTERNAL})
    # need to set global includes property for the module so dependent modules can pick it up
    _argus_set_module_includes("${PROJECT_NAME}" "${INCLUDE_DIRS}")
  else()
    target_compile_definitions("${PROJECT_NAME}" PUBLIC "${MODULE_DEFINITIONS}")

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
      target_compile_definitions("${PROJECT_NAME}" PUBLIC "_ARGUS_DEBUG_MODE")
    endif()

    if(WIN32)
      target_compile_definitions("${PROJECT_NAME}" PUBLIC "GLFW_DLL")
      target_compile_definitions("${PROJECT_NAME}" PUBLIC "NOMINMAX")
    endif()
    
    # recursively load this module's include dirs
    _argus_find_module_includes(PROJECT_INCLUDES "${MODULE_PROJECT_DIR}" "${MODULE_GENERATED_DIR}"
                                "${MODULE_ENGINE_LIB_DEPS}" "${MODULE_ENGINE_MOD_DEPS}")
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
  endif()
endfunction()