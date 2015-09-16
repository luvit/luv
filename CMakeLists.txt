cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0053 OLD)
project (luv C)

option(BUILD_MODULE "Build as module" ON)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(WITH_SHARED_LIBUV "Link to a shared libuv library instead of static linking" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

message("CFLAGS: " ${CFLAGS})
message("LIBFLAG: " ${LIBFLAG})
message("LUA_LIBDIR: " ${LUA_LIBDIR})
message("LUA_BINDIR: " ${LUA_BINDIR})
message("LUA_INCDIR: " ${LUA_INCDIR})
message("LUA: " ${LUA})
message("PREFIX: " ${PREFIX})
message("BINDIR: " ${BINDIR})
message("LIBDIR: " ${LIBDIR})
message("LUADIR: " ${LUADIR})
message("CONFDIR: " ${CONFDIR})

find_package(Lua)

message(STATUS "Lua executable: " ${LUA_EXECUTABLE}) # - the full path to lua
message(STATUS "Lua libraries: " ${LUA_LIBRARIES}) # - the lua shared library
message(STATUS "Lua include dir: " ${LUA_INCLUDE_DIR}) # - directory for lua includes
message(STATUS "Lua package path: " ${LUA_PACKAGE_PATH}) # - where Lua searches for Lua packages
message(STATUS "Lua package cpath: " ${LUA_PACKAGE_CPATH}) # - where Lua searches for library packages

include_directories(${LUA_INCLUDE_DIR})

if (WITH_SHARED_LIBUV)
  find_package(Libuv)
  include_directories(${LIBUV_INCLUDE_DIR})
else ()
  include(deps/uv.cmake)
endif ()

if(APPLE)
  set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS
    "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -flat_namespace -undefined suppress"
  )
endif()

add_library(luv MODULE src/luv.c)
set_target_properties(luv PROPERTIES PREFIX "")


target_link_libraries(luv uv)

if (LIBDIR)
  install(TARGETS luv
    ARCHIVE DESTINATION "${LIBDIR}"
    LIBRARY DESTINATION "${LIBDIR}"
  )
endif()
