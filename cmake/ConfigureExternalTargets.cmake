set(EXT_LIBS_DIR "${CMAKE_SOURCE_DIR}/external/libs")
set(EXT_SPECS_DIR "${CMAKE_SOURCE_DIR}/external/specs")

set(GLFW_SOURCE_DIR "${EXT_LIBS_DIR}/glfw")
set(ZLIB_SOURCE_DIR "${EXT_LIBS_DIR}/zlib")
set(PNG_SOURCE_DIR "${EXT_LIBS_DIR}/libpng")
set(JSON_SOURCE_DIR "${EXT_LIBS_DIR}/json")
set(ARP_SOURCE_DIR "${EXT_LIBS_DIR}/libarp")
set(GLSLANG_SOURCE_DIR "${EXT_LIBS_DIR}/glslang")
set(SPIRV_CROSS_SOURCE_DIR "${EXT_LIBS_DIR}/SPIRV-Cross")
set(LUA_SOURCE_DIR "${EXT_LIBS_DIR}/lua")
set(LUA_BUILDSCRIPT_DIR "${CMAKE_SOURCE_DIR}/cmake/dep/lua")
set(ANGELSCRIPT_SOURCE_DIR "${EXT_LIBS_DIR}/angelscript/sdk/angelscript")

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

# configure libarp
set(LIBARP_FEATURE_PACK OFF CACHE BOOL "" FORCE)
set(LIBARP_USER_MAPPINGS "${PROJECT_SOURCE_DIR}/res/arp_custom_mappings.csv" CACHE STRING "" FORCE)

# configure glslang
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)

# configure SPIRV-Cross
set(SPIRV_CROSS_FORCE_PIC ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)

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

set(GLSLANG_LIBRARY "glslang;SPIRV")
set(GLSLANG_INCLUDE_DIR "${GLSLANG_SOURCE_DIR}")

set(SPIRV_CROSS_LIBRARY "spirv-cross-cpp")
set(SPIRV_CROSS_INCLUDE_DIR "${SPIRV_CROSS_SOURCE_DIR}")

set(LUA_LIBRARY "lua")
set(LUA_INCLUDE_DIR "${LUA_SOURCE_DIR}")

set(ANGELSCRIPT_LIBRARY "angelscript")
set(ANGELSCRIPT_INCLUDE_DIR "${ANGELSCRIPT_SOURCE_DIR}/include;${ANGELSCRIPT_SOURCE_DIR}/../add_on")

# add dependencies
add_subdirectory("${GLFW_SOURCE_DIR}")
add_subdirectory("${ZLIB_SOURCE_DIR}")
add_subdirectory("${PNG_SOURCE_DIR}")
add_subdirectory("${ARP_SOURCE_DIR}")
add_subdirectory("${SPIRV_CROSS_SOURCE_DIR}")
add_subdirectory("${LUA_BUILDSCRIPT_DIR}")
#add_subdirectory("${ANGELSCRIPT_SOURCE_DIR}/projects/cmake")

#_argus_append_source_files(${ANGELSCRIPT_LIBRARY} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptarray")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARY} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptbuilder")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARY} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptstdstring")

# we want to build this lib statically
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("${GLSLANG_SOURCE_DIR}")

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
