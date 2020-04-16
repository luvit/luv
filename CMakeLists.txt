cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # faster evaluation of variable references
endif()

project (luv C ASM)

set(LUV_VERSION_MAJOR 1)
set(LUV_VERSION_MINOR 36)
set(LUV_VERSION_PATCH 0)
set(LUV_VERSION ${LUV_VERSION_MAJOR}.${LUV_VERSION_MINOR}.${LUV_VERSION_PATCH})

option(BUILD_MODULE "Build as module" ON)
option(BUILD_STATIC_LIBS "Build static library" OFF)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(WITH_SHARED_LIBUV "Link to a shared libuv library instead of static linking" OFF)

if (MINGW)
  add_definitions(-D_WIN32_WINNT=0x0600)
endif (MINGW)

if (WIN32)
  # replace /MD to /MT to avoid link msvcr*.dll
  # this needs to be before add_subdirectory calls so that they inherit the modified flags
  set(CompilerFlags
    CMAKE_C_FLAGS
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_MINSIZEREL
    CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_C_FLAGS_RELEASE)
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
endif ()

if (NOT WITH_LUA_ENGINE)
  set(WITH_LUA_ENGINE "LuaJIT"
    CACHE STRING "Link to LuaJIT or PUC Lua" FORCE)
  set_property(CACHE WITH_LUA_ENGINE
    PROPERTY STRINGS  "Lua;LuaJIT")
endif (NOT WITH_LUA_ENGINE)

if (NOT LUA_BUILD_TYPE)
  set(LUA_BUILD_TYPE "Static"
    CACHE STRING "Build Lua/LuaJIT as static, dynamic libary, or use system one" FORCE)
  set_property(CACHE LUA_BUILD_TYPE
    PROPERTY STRINGS  "Static;Dynamic;System")
endif (NOT LUA_BUILD_TYPE)

if (WITH_LUA_ENGINE STREQUAL Lua)
  if (NOT WIN32)
    add_definitions(-DLUA_USE_DLOPEN)
  endif (NOT WIN32)
  set(USE_LUAJIT OFF)
else ()
  set(USE_LUAJIT ON)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

if (WITH_SHARED_LIBUV)
  find_package(Libuv REQUIRED)
  include_directories(${LIBUV_INCLUDE_DIR})
else (WITH_SHARED_LIBUV)
  include_directories(deps/libuv/include)
  add_subdirectory(deps/libuv EXCLUDE_FROM_ALL)
  if (BUILD_MODULE)
    add_definitions( -DBUILDING_UV_SHARED )
  endif (BUILD_MODULE)
  set(LIBUV_LIBRARIES uv_a)
  set_target_properties(uv_a PROPERTIES COMPILE_FLAGS "-fPIC")
endif (WITH_SHARED_LIBUV)

if (LUA)
  MESSAGE(STATUS "Lua: using information from luarocks")

  MESSAGE(STATUS "LUA_LIBDIR: " ${LUA_LIBDIR})
  MESSAGE(STATUS "LUA_INCDIR: " ${LUA_INCDIR})
  MESSAGE(STATUS "LUA: " ${LUA})

  SET(LUA_EXECUTABLE "${LUA}")
  SET(LUA_INCLUDE_DIR "${LUA_INCDIR}")
  SET(LUA_PACKAGE_PATH "${LUADIR}")
  SET(LUA_PACKAGE_CPATH "${LIBDIR}")

  SET(INSTALL_LIB_DIR ${LIBDIR})

  GET_FILENAME_COMPONENT(LUA_EXEC_NAME ${LUA_EXECUTABLE} NAME_WE)
  IF(LUA_EXEC_NAME STREQUAL "luajit")
  FIND_LIBRARY(LUA_LIBRARIES
    NAMES luajit libluajit
    PATHS ${LUA_LIBDIR}
    NO_DEFAULT_PATH)
  ELSEIF(LUA_EXEC_NAME MATCHES "lua.*")
    FIND_LIBRARY(LUA_LIBRARIES
      NAMES lua lua53 lua52 lua51 liblua liblua53 liblua52 liblua51
      PATHS ${LUA_LIBDIR}
      NO_DEFAULT_PATH)
  ENDIF()
  MESSAGE(STATUS "Lua library: ${LUA_LIBRARIES}")

  include_directories(${LUA_INCLUDE_DIR})
else (LUA)
  if (LUA_BUILD_TYPE STREQUAL System)
    if (USE_LUAJIT)
      find_package(LuaJIT REQUIRED)
      include_directories(${LUAJIT_INCLUDE_DIR})
      link_directories(${LUAJIT_LIBRARIES})
    else (USE_LUAJIT)
      find_package(Lua REQUIRED)
      include_directories(${LUA_INCLUDE_DIR})
    endif (USE_LUAJIT)

  else (LUA_BUILD_TYPE STREQUAL System)
    if (LUA_BUILD_TYPE STREQUAL Static)
      SET(WITH_SHARED_LUA OFF)
    else (LUA_BUILD_TYPE STREQUAL Static)
      SET(WITH_SHARED_LUA ON)
    endif (LUA_BUILD_TYPE STREQUAL Static)
    if (USE_LUAJIT)
      include(deps/luajit.cmake)
      include_directories(deps/luajit/src)
      set(LUAJIT_LIBRARIES luajit-5.1)
    else(USE_LUAJIT)
      include(deps/lua.cmake)
      include_directories(deps/lua)
    endif (USE_LUAJIT)
  endif (LUA_BUILD_TYPE STREQUAL System)
endif (LUA)

if (BUILD_MODULE)
  add_library(luv MODULE src/luv.c)
  set_target_properties(luv PROPERTIES PREFIX "")
  list(APPEND ACTIVE_TARGETS "luv")
endif (BUILD_MODULE)
if (BUILD_STATIC_LIBS)
  add_library(libluv_a STATIC src/luv.c)
  set_target_properties(libluv_a PROPERTIES PREFIX "")
  list(APPEND ACTIVE_TARGETS "libluv_a")
endif (BUILD_STATIC_LIBS)
if (BUILD_SHARED_LIBS)
  add_library(libluv SHARED src/luv.c)
  set_target_properties(libluv PROPERTIES PREFIX "")
  set_target_properties(libluv
    PROPERTIES VERSION ${LUV_VERSION} SOVERSION ${LUV_VERSION_MAJOR})
  list(APPEND ACTIVE_TARGETS "libluv")
endif (BUILD_SHARED_LIBS)

if(APPLE)
  set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS
    "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -flat_namespace -undefined suppress"
  )
  # execute_process(COMMAND which luajit OUTPUT_VARIABLE LUAJIT)
  # set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS
  #   "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -bundle_loader ${LUAJIT}"
  # )
endif()

if(NOT LUA_COMPAT53_DIR)
  set(LUA_COMPAT53_DIR deps/lua-compat-5.3)
endif()
if(DEFINED ENV{LUA_COMPAT53_DIR})
  set(LUA_COMPAT53_DIR $ENV{LUA_COMPAT53_DIR})
endif()
include_directories(${LUA_COMPAT53_DIR}/c-api)

if(WIN32)
  add_definitions(-DLUA_BUILD_AS_DLL -DLUA_LIB)
endif()

foreach(TARGET_NAME ${ACTIVE_TARGETS})
  if(WIN32)
    if (LUA)
      target_link_libraries(${TARGET_NAME} ${LIBUV_LIBRARIES} ${LUA_LIBRARIES})
    else (LUA)
      if (USE_LUAJIT)
        target_link_libraries(${TARGET_NAME} ${LIBUV_LIBRARIES} ${LUAJIT_LIBRARIES})
      else (USE_LUAJIT)
        if (LUA_BUILD_TYPE STREQUAL System)
            target_link_libraries(${TARGET_NAME} ${LIBUV_LIBRARIES} ${LUA_LIBRARIES})
        else (LUA_BUILD_TYPE STREQUAL System)
            target_link_libraries(${TARGET_NAME} ${LIBUV_LIBRARIES} lualib)
        endif (LUA_BUILD_TYPE STREQUAL System)
      endif (USE_LUAJIT)
    endif (LUA)
  elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    target_link_libraries(${TARGET_NAME} ${LIBUV_LIBRARIES} rt)
  else()
    target_link_libraries(${TARGET_NAME} ${LIBUV_LIBRARIES})
  endif()
endforeach()

if (NOT LUA)
  if (BUILD_MODULE)
    if (WIN32)
      set(MODULE_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    else (WIN32)
      set(MODULE_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
    endif (WIN32)
  endif (BUILD_MODULE)
  if (BUILD_STATIC_LIBS)
    set(STATICLIBS_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib"
      CACHE PATH "Installation directory for static libraries")
  endif (BUILD_STATIC_LIBS)
  if (BUILD_SHARED_LIBS)
    set(SHAREDLIBS_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib"
      CACHE PATH "Installation directory for shared libraries")
  endif (BUILD_SHARED_LIBS)
else ()
  # use paths from luaRocks
  set(MODULE_INSTALL_LIB_DIR "${INSTALL_LIB_DIR}")
  set(STATICLIBS_INSTALL_LIB_DIR "${INSTALL_LIB_DIR}")
  set(SHAREDLIBS_INSTALL_LIB_DIR "${INSTALL_LIB_DIR}")
endif ()

# header install paths are LuaRocks-agnostic, so just use CMAKE_INSTALL_PREFIX regardless
if (BUILD_STATIC_LIBS)
  set(STATICLIBS_INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include/luv"
    CACHE PATH "Installation directory for headers")
endif (BUILD_STATIC_LIBS)
if (BUILD_SHARED_LIBS)
  set(SHAREDLIBS_INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include/luv"
    CACHE PATH "Installation directory for headers")
endif (BUILD_SHARED_LIBS)

if (CMAKE_INSTALL_PREFIX)
  if (BUILD_MODULE)
    install(TARGETS luv
      ARCHIVE DESTINATION "${MODULE_INSTALL_LIB_DIR}"
      LIBRARY DESTINATION "${MODULE_INSTALL_LIB_DIR}"
    )
  endif (BUILD_MODULE)
  if (BUILD_STATIC_LIBS)
    install(TARGETS libluv_a
      ARCHIVE DESTINATION "${STATICLIBS_INSTALL_LIB_DIR}"
      LIBRARY DESTINATION "${STATICLIBS_INSTALL_LIB_DIR}"
    )
    install(
      FILES src/luv.h src/util.h src/lhandle.h src/lreq.h
      DESTINATION "${STATICLIBS_INSTALL_INC_DIR}"
    )
  endif (BUILD_STATIC_LIBS)
  if (BUILD_SHARED_LIBS)
    install(TARGETS libluv
      ARCHIVE DESTINATION "${SHAREDLIBS_INSTALL_LIB_DIR}"
      LIBRARY DESTINATION "${SHAREDLIBS_INSTALL_LIB_DIR}"
    )
    if(UNIX)
      set(prefix ${CMAKE_INSTALL_PREFIX})
      set(includedir ${SHAREDLIBS_INSTALL_INC_DIR})
      set(libdir ${SHAREDLIBS_INSTALL_LIB_DIR})
      configure_file(libluv.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libluv.pc @ONLY)
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libluv.pc
        DESTINATION ${SHAREDLIBS_INSTALL_LIB_DIR}/pkgconfig)
    endif()
    install(
      FILES src/luv.h src/util.h src/lhandle.h src/lreq.h
      DESTINATION "${SHAREDLIBS_INSTALL_INC_DIR}"
    )
  endif (BUILD_SHARED_LIBS)
endif (CMAKE_INSTALL_PREFIX)
