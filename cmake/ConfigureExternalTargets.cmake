set(EXT_LIBS_DIR "${CMAKE_SOURCE_DIR}/external/libs")
set(EXT_SPECS_DIR "${CMAKE_SOURCE_DIR}/external/specs")

# disable CTest for external projects
set(ENABLE_CTEST_SAVED "${ENABLE_CTEST}")
set(ENABLE_CTEST OFF)

set(SDL2_SOURCE_DIR "${EXT_LIBS_DIR}/sdl2")
set(ZLIB_SOURCE_DIR "${EXT_LIBS_DIR}/zlib")
set(PNG_SOURCE_DIR "${EXT_LIBS_DIR}/libpng")
set(JSON_SOURCE_DIR "${EXT_LIBS_DIR}/json")
set(ARP_SOURCE_DIR "${EXT_LIBS_DIR}/libarp")
set(GLSLANG_SOURCE_DIR "${EXT_LIBS_DIR}/glslang")
set(SPIRV_CROSS_SOURCE_DIR "${EXT_LIBS_DIR}/SPIRV-Cross")
set(LUA_SOURCE_DIR "${EXT_LIBS_DIR}/lua")
set(LUA_BUILDSCRIPT_DIR "${CMAKE_SOURCE_DIR}/cmake/dep/lua")
set(ANGELSCRIPT_SOURCE_DIR "${EXT_LIBS_DIR}/angelscript/sdk/angelscript")
set(CATCH2_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/test/libs/catch2")

# disable SDL render subsystem and tests
set(SDL_Render OFF)
set(SDL_TEST OFF)

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

# add dependencies

# SDL2
set(SDL2_LIBRARY "SDL2")
set(SDL2_LIBRARIES "${SDL2_LIBRARY}")
set(SDL2_INCLUDE_DIRS "${SDL2_SOURCE_DIR}/include")
add_subdirectory("${SDL2_SOURCE_DIR}")

# zlib
set(ZLIB_LIBRARY "zlib")
set(ZLIB_LIBRARIES "${ZLIB_LIBRARY}")
set(ZLIB_INCLUDE_DIRS "${ZLIB_SOURCE_DIR};${TMP_INCLUDE_DIR}/zlib")
set(ZLIB_INCLUDE_DIR "${ZLIB_INCLUDE_DIRS}")
add_subdirectory("${ZLIB_SOURCE_DIR}")

# libpng
set(PNG_LIBRARY "png")
set(PNG_LIBRARIES "${PNG_LIBRARY}")
set(PNG_INCLUDE_DIRS "${PNG_SOURCE_DIR};${TMP_INCLUDE_DIR}/libpng")
set(PNG_INCLUDE_DIR "${PNG_INCLUDE_DIRS}")
set(PNG_BUILD_ZLIB ON CACHE BOOL "" FORCE)
add_subdirectory("${PNG_SOURCE_DIR}")

# json
set(JSON_INCLUDE_DIRS "${JSON_SOURCE_DIR}/include")
set(JSON_INCLUDE_DIR "${JSON_INCLUDE_DIRS}")
# header-only library, nothing to include here

# libarp
set(ARP_LIBRARY "arp")
set(ARP_LIBRARIES "${ARP_LIBRARY};${ZLIB_LIBRARIES}")
set(ARP_INCLUDE_DIRS "${ARP_SOURCE_DIR}/include")
set(ARP_INCLUDE_DIR "${ARP_INCLUDE_DIRS}")
add_subdirectory("${ARP_SOURCE_DIR}")

# glslang
set(GLSLANG_LIBRARY "glslang")
set(GLSLANG_LIBRARIES "${GLSLANG_LIBRARY};SPIRV")
set(GLSLANG_INCLUDE_DIRS "${GLSLANG_SOURCE_DIR}")
set(GLSLANG_INCLUDE_DIR "${GLSLANG_INCLUDE_DIRS}")
# we want to build this lib statically
set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory("${GLSLANG_SOURCE_DIR}")
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

# SPIRV-Cross
set(SPIRV_CROSS_LIBRARY "spirv-cross-cpp")
set(SPIRV_CROSS_LIBRARIES "${SPIRV_CROSS_LIBRARY}")
set(SPIRV_CROSS_INCLUDE_DIRS "${SPIRV_CROSS_SOURCE_DIR}")
set(SPIRV_CROSS_INCLUDE_DIR "${SPIRV_CROSS_INCLUDE_DIRS}")
add_subdirectory("${SPIRV_CROSS_SOURCE_DIR}")

# Lua
set(LUA_LIBRARY "lua")
set(LUA_LIBRARIES "${LUA_LIBRARY}")
set(LUA_INCLUDE_DIRS "${LUA_SOURCE_DIR}")
set(LUA_INCLUDE_DIR "${LUA_INCLUDE_DIR}")
add_subdirectory("${LUA_BUILDSCRIPT_DIR}")

# Angelscript
set(ANGELSCRIPT_LIBRARY "angelscript")
set(ANGELSCRIPT_LIBRARIES "${ANGELSCRIPT_LIBRARY}")
set(ANGELSCRIPT_INCLUDE_DIRS "${ANGELSCRIPT_SOURCE_DIR}/include;${ANGELSCRIPT_SOURCE_DIR}/../add_on")
set(ANGELSCRIPT_INCLUDE_DIR "${ANGELSCRIPT_INCLUDE_DIRS}")
#add_subdirectory("${ANGELSCRIPT_SOURCE_DIR}/projects/cmake")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARIES} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptarray")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARIES} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptbuilder")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARIES} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptstdstring")

# add test dependencies

# set build variables for test dependencies
set(CATCH2_LIBRARY "Catch2")
set(CATCH2_LIBRARIES "${CATCH2_LIBRARY}")
set(CATCH2_INCLUDE_DIRS "${CATCH2_SOURCE_DIR}/src")
set(CATCH2_INCLUDE_DIR "${CATCH2_INCLUDE_DIRS}")
# we want to build this lib statically
set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory("${CATCH2_SOURCE_DIR}")
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

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

# disable warnings for subprojects
_argus_disable_warnings(${SDL2_LIBRARY})
_argus_disable_warnings(${ZLIB_LIBRARY})
_argus_disable_warnings(${PNG_LIBRARY})
_argus_disable_warnings(${GLSLANG_LIBRARY})
_argus_disable_warnings(${SPIRV_CROSS_LIBRARY})
_argus_disable_warnings(${LUA_LIBRARY})
#_argus_disable_warnings(${ANGELSCRIPT_LIBRARY})
_argus_disable_warnings(${CATCH2_LIBRARY})

# pop original value of ENABLE_CTEST option
set(ENABLE_CTEST "${ENABLE_CTEST_SAVED}")
