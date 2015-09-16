# - Find lua
# this module looks for Lua
#
#  LUA_EXECUTABLE - the full path to lua
#  LUA_LIBRARIES - the lua shared library
#  LUA_INCLUDE_DIR - directory for lua includes
#  LUA_PACKAGE_PATH - where Lua searches for Lua packages
#  LUA_PACKAGE_CPATH - where Lua searches for library packages
#  LUA_FOUND      - If false, don't attempt to use lua.

INCLUDE(CheckLibraryExists)

IF(LUA) # if we are using luarocks
  MESSAGE(STATUS "Lua: using information from luarocks")
  SET(LUA_EXECUTABLE "${LUA}")
  SET(LUA_INCLUDE_DIR "${LUA_INCDIR}")
  SET(LUA_PACKAGE_PATH "${LUADIR}")
  SET(LUA_PACKAGE_CPATH "${LIBDIR}")

  IF(LUALIB) # present on windows platforms only
    SET(LUA_LIBRARIES "${LUALIB}")
  ELSE() # too bad, luarocks does not provide it (pfff...)
    GET_FILENAME_COMPONENT(LUA_EXEC_NAME ${LUA_EXECUTABLE} NAME_WE)
    IF(LUA_EXEC_NAME STREQUAL "luajit")
    FIND_LIBRARY(LUA_LIBRARIES
      NAMES luajit libluajit
      PATHS ${LUA_LIBDIR}
      NO_DEFAULT_PATH)
    ELSEIF(LUA_EXEC_NAME STREQUAL "lua")
      FIND_LIBRARY(LUA_LIBRARIES
        NAMES lua liblua
        PATHS ${LUA_LIBDIR}
        NO_DEFAULT_PATH)
    ELSE()
      MESSAGE(FATAL_ERROR "You seem to have a non-standard lua installation -- are you using luajit-rocks?")
    ENDIF()
    MESSAGE(STATUS "Lua library guess (no info from luarocks): ${LUA_LIBRARIES}")
  ENDIF()

ELSE() # standalone -- not using luarocks

  IF(WITH_LUA51)
    FIND_PROGRAM(LUA_EXECUTABLE
      NAMES
      lua51
      lua5.1
      lua
      PATH)
  ELSE()
    FIND_PROGRAM(LUA_EXECUTABLE
      NAMES
      luajit
      PATH)
  ENDIF()
  MESSAGE(STATUS "Lua executable: ${LUA_EXECUTABLE}")

  IF(LUA_EXECUTABLE)
    GET_FILENAME_COMPONENT(LUA_DIR ${LUA_EXECUTABLE} PATH)
  ENDIF(LUA_EXECUTABLE)

  IF(WITH_LUA51)
    FIND_LIBRARY(LUA_LIBRARIES
      NAMES lua liblua lua-5.1 liblua-5.1 libluajit-5.1
      PATHS ${LUA_DIR}/../lib
      ${LUA_DIR}
      NO_DEFAULT_PATH)
  ELSE()
    FIND_LIBRARY(LUA_LIBRARIES
      NAMES luajit libluajit luajit-5.1 libluajit-5.1
      PATHS ${LUA_DIR}/../lib
      ${LUA_DIR}
      NO_DEFAULT_PATH)
  ENDIF()
  MESSAGE(STATUS "Lua library: ${LUA_LIBRARIES}")

  FIND_PATH(LUA_INCLUDE_DIR lua.h
    ${LUA_DIR}/../include/lua-5.1
    ${LUA_DIR}/../include/lua51
    ${LUA_DIR}/../include/lua
    ${LUA_DIR}/../include
    NO_DEFAULT_PATH)

  MESSAGE(STATUS "Lua include directory: ${LUA_INCLUDE_DIR}")

ENDIF()

IF(LUA_LIBRARIES)
  CHECK_LIBRARY_EXISTS(${LUA_LIBRARIES} luaJIT_setmode "" LUA_JIT)
ENDIF()

MARK_AS_ADVANCED(
  LUA_EXECUTABLE
  LUA_LIBRARIES
  LUA_INCLUDE_DIR
  LUA_PACKAGE_PATH
  LUA_PACKAGE_CPATH
  )

IF(LUA_EXECUTABLE)
  IF(LUA_LIBRARIES)
    IF(LUA_INCLUDE_DIR)
      SET(LUA_FOUND 1)
    ENDIF(LUA_INCLUDE_DIR)
  ENDIF(LUA_LIBRARIES)
ENDIF(LUA_EXECUTABLE)

IF (NOT LUA_FOUND AND Lua_FIND_REQUIRED)
  MESSAGE(FATAL_ERROR "Could not find Lua")
ENDIF (NOT LUA_FOUND AND Lua_FIND_REQUIRED)

IF(NOT Lua_FIND_QUIETLY)
  IF(LUA_FOUND)
    MESSAGE(STATUS "Lua found ${LUA_EXECUTABLE}")
  ELSE(LUA_FOUND)
    MESSAGE(STATUS "Lua not found. Please specify location")
  ENDIF(LUA_FOUND)
ENDIF(NOT Lua_FIND_QUIETLY)
