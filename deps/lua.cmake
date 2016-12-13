# Modfied from luajit.cmake
# Added LUA_ADD_EXECUTABLE Ryan Phillips <ryan at trolocsis.com>
# This CMakeLists.txt has been first taken from LuaDist
# Copyright (C) 2007-2011 LuaDist.
# Created by Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# Debugged and (now seriously) modified by Ronan Collobert, for Torch7

#project(Lua53 C)

SET(LUA_DIR ${CMAKE_CURRENT_LIST_DIR}/lua CACHE PATH "location of lua sources")

SET(CMAKE_REQUIRED_INCLUDES
  ${LUA_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

OPTION(WITH_AMALG "Build eveything in one shot (needs memory)" ON)

# Ugly warnings
IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

# Various includes
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckTypeSize)

CHECK_TYPE_SIZE("void*" SIZEOF_VOID_P)
IF(SIZEOF_VOID_P EQUAL 8)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
ENDIF()

IF(NOT WIN32)
  FIND_LIBRARY(DL_LIBRARY "dl")
  IF(DL_LIBRARY)
    SET(CMAKE_REQUIRED_LIBRARIES ${DL_LIBRARY})
    LIST(APPEND LIBS ${DL_LIBRARY})
  ENDIF(DL_LIBRARY)
  CHECK_FUNCTION_EXISTS(dlopen LUA_USE_DLOPEN)
  IF(NOT LUA_USE_DLOPEN)
    MESSAGE(FATAL_ERROR "Cannot compile a useful lua.
Function dlopen() seems not to be supported on your platform.
Apparently you are not on a Windows platform as well.
So lua has no way to deal with shared libraries!")
  ENDIF(NOT LUA_USE_DLOPEN)
ENDIF(NOT WIN32)

check_library_exists(m sin "" LUA_USE_LIBM)
if ( LUA_USE_LIBM )
  list ( APPEND LIBS m )
endif ()

## SOURCES
SET(SRC_LUALIB
  ${LUA_DIR}/lbaselib.c
  ${LUA_DIR}/lcorolib.c
  ${LUA_DIR}/ldblib.c
  ${LUA_DIR}/liolib.c
  ${LUA_DIR}/lmathlib.c
  ${LUA_DIR}/loadlib.c
  ${LUA_DIR}/loslib.c
  ${LUA_DIR}/lstrlib.c
  ${LUA_DIR}/ltablib.c
  ${LUA_DIR}/lutf8lib.c)

SET(SRC_LUACORE
  ${LUA_DIR}/lauxlib.c
  ${LUA_DIR}/lapi.c
  ${LUA_DIR}/lcode.c
  ${LUA_DIR}/lctype.c
  ${LUA_DIR}/ldebug.c
  ${LUA_DIR}/ldo.c
  ${LUA_DIR}/ldump.c
  ${LUA_DIR}/lfunc.c
  ${LUA_DIR}/lgc.c
  ${LUA_DIR}/linit.c
  ${LUA_DIR}/llex.c
  ${LUA_DIR}/lmem.c
  ${LUA_DIR}/lobject.c
  ${LUA_DIR}/lopcodes.c
  ${LUA_DIR}/lparser.c
  ${LUA_DIR}/lstate.c
  ${LUA_DIR}/lstring.c
  ${LUA_DIR}/ltable.c
  ${LUA_DIR}/ltm.c
  ${LUA_DIR}/lundump.c
  ${LUA_DIR}/lvm.c
  ${LUA_DIR}/lzio.c
  ${SRC_LUALIB})

## GENERATE

IF(WITH_SHARED_LUA)
  IF(WITH_AMALG)
    add_library(lualib SHARED ${LUA_DIR}/../lua_one.c)
  ELSE()
    add_library(lualib SHARED ${SRC_LUACORE})
  ENDIF()
ELSE()
  IF(WITH_AMALG)
    add_library(lualib STATIC ${LUA_DIR}/../lua_one.c )
  ELSE()
    add_library(lualib STATIC ${SRC_LUACORE} )
  ENDIF()
  set_target_properties(lualib PROPERTIES
    PREFIX "lib" IMPORT_PREFIX "lib")
ENDIF()

target_link_libraries (lualib ${LIBS} )
set_target_properties (lualib PROPERTIES OUTPUT_NAME "lua53")

add_executable(lua ${LUA_DIR}/lua.c)
IF(WIN32)
  target_link_libraries(lua lualib)
ELSE()
  target_link_libraries(lua lualib ${LIBS})
  SET_TARGET_PROPERTIES(lua PROPERTIES ENABLE_EXPORTS ON)
ENDIF(WIN32)

MACRO(LUA_add_custom_commands luajit_target)
  SET(target_srcs "")
  FOREACH(file ${ARGN})
    IF(${file} MATCHES ".*\\.lua$")
      set(file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
      set(source_file ${file})
      string(LENGTH ${CMAKE_SOURCE_DIR} _luajit_source_dir_length)
      string(LENGTH ${file} _luajit_file_length)
      math(EXPR _begin "${_luajit_source_dir_length} + 1")
      math(EXPR _stripped_file_length "${_luajit_file_length} - ${_luajit_source_dir_length} - 1")
      string(SUBSTRING ${file} ${_begin} ${_stripped_file_length} stripped_file)

      set(generated_file "${CMAKE_BINARY_DIR}/luacode_tmp/${stripped_file}_${luajit_target}_generated.c")

      add_custom_command(
        OUTPUT ${generated_file}
        MAIN_DEPENDENCY ${source_file}
        DEPENDS lua
        COMMAND lua
	ARGS "${LUA_DIR}/../luac.lua"
          ${source_file}
          ${generated_file}
        COMMENT "Building Lua ${source_file}: ${generated_file}"
      )

      get_filename_component(basedir ${generated_file} PATH)
      file(MAKE_DIRECTORY ${basedir})

      set(target_srcs ${target_srcs} ${generated_file})
      set_source_files_properties(
        ${generated_file}
        properties
        generated true        # to say that "it is OK that the obj-files do not exist before build time"
      )
    ELSE()
      set(target_srcs ${target_srcs} ${file})
    ENDIF(${file} MATCHES ".*\\.lua$")
  ENDFOREACH(file)
ENDMACRO()

MACRO(LUA_ADD_EXECUTABLE luajit_target)
  LUA_add_custom_commands(${luajit_target} ${ARGN})
  add_executable(${luajit_target} ${target_srcs})
ENDMACRO(LUA_ADD_EXECUTABLE luajit_target)
