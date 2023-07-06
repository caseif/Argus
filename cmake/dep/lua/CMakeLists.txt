cmake_minimum_required(VERSION 3.17)
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW) # use <package>_ROOT env var as hint
endif()

project(lua
    LANGUAGES C
    VERSION 5.4.6)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_DEBUG_POSTFIX "d")

set(SOURCES_DIR "../../../external/libs/lua")

set(C_FILES "lapi;lcode;lctype;ldebug;ldo;ldump;lfunc;"
             "lgc;llex;lmem;lobject;lopcodes;lparser;lstate;"
             "lstring;ltable;ltm;lundump;lvm;lzio;ltests;"
             "lauxlib;lbaselib;ldblib;liolib;lmathlib;loslib;"
             "ltablib;lstrlib;lutf8lib;loadlib;lcorolib;linit")
list(TRANSFORM C_FILES REPLACE "^(.+)$" "${SOURCES_DIR}/\\1.c")
add_library("${PROJECT_NAME}" SHARED "${C_FILES}")
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 99)
target_include_directories("${PROJECT_NAME}" PRIVATE "${SOURCES_DIR}")
if(WIN32)
  target_compile_options("${PROJECT_NAME}" PRIVATE
      "/W4"     # -Wall
      "/GS-"    # -fno-stack-protector
      "/we4456" # -Wshadow
      "/wd4668" # -Wundef
      "/wd4505" # -Wredundant-decls
      )
else()
  target_compile_options("${PROJECT_NAME}" PRIVATE
      "-Wall"
      "-fno-stack-protector"
      "-fno-common"
      "-Wfatal-errors"
      "-Wextra"
      "-Wshadow"
      "-Wundef"
      "-Wwrite-strings"
      "-Wredundant-decls"
      "-Wdisabled-optimization"
      "-Wdouble-promotion"
      "-Wmissing-declarations"
      "$<$<CXX_COMPILER_ID:GNU>:-Wlogical-op>"
      "$<$<CXX_COMPILER_ID:GNU>:-Wno-aggressive-loop-optimizations>")
endif()

message("${PROJECT_NAME}")
if(UNIX)
  target_compile_definitions("${PROJECT_NAME}" PRIVATE "-DLUA_USE_LINUX -DLUA_USE_READLINE")
  target_link_options("${PROJECT_NAME}" PRIVATE "-Wl,-E")
  target_link_libraries("${PROJECT_NAME}" PRIVATE "m" "dl" "readline")
endif()

set_target_properties("${PROJECT_NAME}" PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties("${PROJECT_NAME}" PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
