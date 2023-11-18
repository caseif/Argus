include(CheckLibraryExists)

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
set(SPIRV_HEADERS_SOURCE_DIR "${EXT_LIBS_DIR}/SPIRV-Headers")
set(SPIRV_TOOLS_SOURCE_DIR "${EXT_LIBS_DIR}/SPIRV-Tools")
set(GLSLANG_SOURCE_DIR "${EXT_LIBS_DIR}/glslang")
set(SPIRV_CROSS_SOURCE_DIR "${EXT_LIBS_DIR}/SPIRV-Cross")
set(LUA_SOURCE_DIR "${EXT_LIBS_DIR}/lua")
set(LUA_BUILDSCRIPT_DIR "${CMAKE_SOURCE_DIR}/cmake/dep/lua")
set(ANGELSCRIPT_SOURCE_DIR "${EXT_LIBS_DIR}/angelscript/sdk/angelscript")
set(CATCH2_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/test/libs/catch2")

# disable SDL render subsystem and tests
set(SDL_Render OFF)
set(SDL_TEST OFF)

if(NOT USE_SYSTEM_PNG)
  # don't let libpng try to install itself
  set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
  # configure libpng build steps
  set(PNG_STATIC OFF CACHE BOOL "" FORCE)
  set(PNG_TESTS OFF CACHE BOOL "" FORCE)
endif()

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
if(NOT USE_SYSTEM_PNG)
  configure_file("${PNG_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt" "${TMP_INCLUDE_DIR}/libpng/pnglibconf.h")
endif()
if(NOT USE_SYSTEM_ZLIB)
  configure_file("${ZLIB_SOURCE_DIR}/zconf.h.in" "${TMP_INCLUDE_DIR}/zlib/zconf.h")
endif()

if(NOT DEFINED UNIX)
  set(UNIX 0)
endif()

# set install prefix explicitly since a couple of the dependency projects like
# to clobber it when it's not changed from the default value
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

# set relevant build variables so the dependencies can be discovered
# note that we include the dirs for generated headers

# add dependencies

# SDL2
if(USE_SYSTEM_SDL2)
  find_package(SDL2 REQUIRED)
  set(SDL2_LIBRARY "${SDL2_LIBRARIES}")
else()
  set(SDL2_TARGET "SDL2")
  set(SDL2_LIBRARY "${SDL2_TARGET}")
  set(SDL2_LIBRARIES "${SDL2_LIBRARY}")
  set(SDL2_INCLUDE_DIRS "${SDL2_SOURCE_DIR}/include")
  add_subdirectory("${SDL2_SOURCE_DIR}")
endif()

# zlib
if(USE_SYSTEM_ZLIB)
  find_package(ZLIB REQUIRED)
else()
  set(ZLIB_TARGET "zlib")
  set(ZLIB_STATIC_TARGET "zlibstatic")
  set(ZLIB_LIBRARY "${ZLIB_TARGET}")
  set(ZLIB_LIBRARIES "${ZLIB_LIBRARY}")
  set(ZLIB_INCLUDE_DIRS "${ZLIB_SOURCE_DIR};${TMP_INCLUDE_DIR}/zlib")
  set(ZLIB_INCLUDE_DIR "${ZLIB_INCLUDE_DIRS}")
  add_subdirectory("${ZLIB_SOURCE_DIR}")
endif()

# libpng
if(USE_SYSTEM_PNG)
  find_package(PNG REQUIRED)
else()
  set(PNG_TARGET "png")
  set(PNG_LIBRARY "${PNG_TARGET}")
  set(PNG_LIBRARIES "${PNG_LIBRARY}")
  set(PNG_INCLUDE_DIRS "${PNG_SOURCE_DIR};${TMP_INCLUDE_DIR}/libpng")
  set(PNG_INCLUDE_DIR "${PNG_INCLUDE_DIRS}")
  if(NOT USE_SYSTEM_ZLIB)
    # indicate that we're building zlib as a subproject
    set(PNG_BUILD_ZLIB ON CACHE BOOL "" FORCE)
  endif()
  add_subdirectory("${PNG_SOURCE_DIR}")
endif()

# json
if(USE_SYSTEM_JSON)
  find_package(nlohmann_json REQUIRED)
  get_target_property(JSON_INCLUDE_DIRS nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
  set(JSON_INCLUDE_DIR "${JSON_INCLUDE_DIRS}")
else()
  set(JSON_INCLUDE_DIRS "${JSON_SOURCE_DIR}/include")
  set(JSON_INCLUDE_DIR "${JSON_INCLUDE_DIRS}")
  # header-only library, don't need to specify any libs
endif()

# libarp
set(ARP_TARGET "arp")
set(ARP_LIBRARY "${ARP_TARGET}")
set(ARP_LIBRARIES "${ARP_LIBRARY};${ZLIB_LIBRARIES}")
set(ARP_INCLUDE_DIRS "${ARP_SOURCE_DIR}/include")
set(ARP_INCLUDE_DIR "${ARP_INCLUDE_DIRS}")
add_subdirectory("${ARP_SOURCE_DIR}")

# we want to build the next several libs statically
set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)

# SPIRV-Headers (required by SPIRV-Tools)
set(SPIRV_HEADERS_TARGET "SPIRV-Headers")
set(SPIRV_HEADERS_LIBRARY "${SPIRV_HEADERS_TARGET}")
set(SPIRV_HEADERS_LIBRARIES "${SPIRV_HEADERS_LIBRARY}")
add_subdirectory("${SPIRV_HEADERS_SOURCE_DIR}")
get_target_property(SPIRV_HEADERS_INCLUDE_DIRS "${SPIRV_HEADERS_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES)
set(SPIRV_HEADERS_INCLUDE_DIR "${SPIRV_HEADERS_INCLUDE_DIRS}")

# SPIRV-Tools (required by glslang for SPIR-V optimization)
set(SPIRV_TOOLS_TARGET "SPIRV-Tools")
set(SPIRV_TOOLS_LIBRARY "${SPIRV_TOOLS_TARGET}")
set(SPIRV_TOOLS_LIBRARIES "${SPIRV_LIBRARIES}")
add_subdirectory("${SPIRV_TOOLS_SOURCE_DIR}")
get_target_property(SPIRV_TOOLS_INCLUDE_DIRS "${SPIRV_TOOLS_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES)
set(SPIRV_TOOLS_INCLUDE_DIR "${SPIRV_TOOLS_INCLUDE_DIRS}")

# glslang
set(GLSLANG_TARGET "glslang")
set(GLSLANG_LIBRARY "${GLSLANG_TARGET}")
set(GLSLANG_LIBRARIES "${GLSLANG_LIBRARY};SPIRV")
set(GLSLANG_INCLUDE_DIRS "${GLSLANG_SOURCE_DIR}")
set(GLSLANG_INCLUDE_DIR "${GLSLANG_INCLUDE_DIRS}")
add_subdirectory("${GLSLANG_SOURCE_DIR}")

# SPIRV-Cross
set(SPIRV_CROSS_TARGET "spirv-cross-cpp")
set(SPIRV_CROSS_LIBRARY "${SPIRV_CROSS_TARGET}")
set(SPIRV_CROSS_LIBRARIES "${SPIRV_CROSS_LIBRARY}")
set(SPIRV_CROSS_INCLUDE_DIRS "${SPIRV_CROSS_SOURCE_DIR}")
set(SPIRV_CROSS_INCLUDE_DIR "${SPIRV_CROSS_INCLUDE_DIRS}")
add_subdirectory("${SPIRV_CROSS_SOURCE_DIR}")

# pop original value for BUILD_SHARED_LIBS
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

# Lua
if(USE_SYSTEM_LUA)
  find_package(Lua REQUIRED)
  set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
else()
  set(LUA_TARGET "lua")
  set(LUA_LIBRARY "${LUA_TARGET}")
  set(LUA_LIBRARIES "${LUA_LIBRARY}")
  set(LUA_INCLUDE_DIRS "${LUA_SOURCE_DIR}")
  set(LUA_INCLUDE_DIR "${LUA_INCLUDE_DIR}")
  add_subdirectory("${LUA_BUILDSCRIPT_DIR}")
endif()

# Angelscript
set(ANGELSCRIPT_TARGET "angelscript")
set(ANGELSCRIPT_LIBRARY "${ANGELSCRIPT_TARGET}")
set(ANGELSCRIPT_LIBRARIES "${ANGELSCRIPT_LIBRARY}")
set(ANGELSCRIPT_INCLUDE_DIRS "${ANGELSCRIPT_SOURCE_DIR}/include;${ANGELSCRIPT_SOURCE_DIR}/../add_on")
set(ANGELSCRIPT_INCLUDE_DIR "${ANGELSCRIPT_INCLUDE_DIRS}")
#add_subdirectory("${ANGELSCRIPT_SOURCE_DIR}/projects/cmake")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARIES} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptarray")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARIES} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptbuilder")
#_argus_append_source_files(${ANGELSCRIPT_LIBRARIES} "${ANGELSCRIPT_SOURCE_DIR}/../add_on/scriptstdstring")

# add test dependencies

# set build variables for test dependencies
set(CATCH2_TARGET "Catch2")
set(CATCH2_LIBRARY "${CATCH2_TARGET}")
set(CATCH2_LIBRARIES "${CATCH2_LIBRARY}")
set(CATCH2_INCLUDE_DIRS "${CATCH2_SOURCE_DIR}/src")
set(CATCH2_INCLUDE_DIR "${CATCH2_INCLUDE_DIRS}")
# we want to build this lib statically
set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory("${CATCH2_SOURCE_DIR}")
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

if(TARGET "example")
  set_target_properties(example PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET "example64")
  set_target_properties(example64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET "minigzip")
  set_target_properties(minigzip PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET "minigzip64")
  set_target_properties(minigzip64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET "pngfix")
  set_target_properties(pngfix PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET "png-fix-itxt")
  set_target_properties(png-fix-itxt PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# disable warnings for subprojects
if(TARGET "${SDL2_TARGET}")
  _argus_disable_warnings(${SDL2_TARGET})
endif()
if(TARGET "${ZLIB_TARGET}")
  _argus_disable_warnings(${ZLIB_TARGET})
endif()
if(TARGET "${ZLIB_STATIC_TARGET}")
  _argus_disable_warnings(${ZLIB_STATIC_TARGET})
endif()
if(TARGET "${PNG_TARGET}")
  _argus_disable_warnings(${PNG_TARGET})
endif()
_argus_disable_warnings(${GLSLANG_TARGET})
_argus_disable_warnings(${SPIRV_CROSS_TARGET})
if(TARGET "${LUA_TARGET}")
  _argus_disable_warnings(${LUA_TARGET})
endif()
#_argus_disable_warnings(${ANGELSCRIPT_LIBRARY})
_argus_disable_warnings(${CATCH2_TARGET})

# pop original value of ENABLE_CTEST option
set(ENABLE_CTEST "${ENABLE_CTEST_SAVED}")
