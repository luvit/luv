cmake_minimum_required(VERSION 2.8)

project (luv)

option(WITH_SHARED_LIBUV "Link to a shared libuv library instead of static linking" OFF)
option(WITH_SHARED_LUAJIT "Link to a shared LuaJIT library instead of static linking" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

if (WITH_SHARED_LIBUV)
  find_package(Libuv)
  if (LIBUV_FOUND)
    include_directories(${LIBUV_INCLUDE_DIR})
  endif (LIBUV_FOUND)
else (WITH_SHARED_LIBUV)
  include(deps/uv.cmake)
endif (WITH_SHARED_LIBUV)

if (WITH_SHARED_LUAJIT)
  find_package(LuaJIT)
  if (LUAJIT_FOUND)
    include_directories(${LUAJIT_INCLUDE_DIR})
    link_directories(${LUAJIT_LIBRARIES})
  endif (LUAJIT_FOUND)
else (WITH_SHARED_LUAJIT)
  include(deps/luajit.cmake)
  include_directories(deps/luajit/src)
endif (WITH_SHARED_LUAJIT)

add_library (luv MODULE src/luv.c)

set_target_properties(luv PROPERTIES OUTPUT_NAME "luv")
set_target_properties(luv PROPERTIES PREFIX "")

if(APPLE)
  set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS
    "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -flat_namespace -undefined suppress"
  )
  # execute_process(COMMAND which luajit OUTPUT_VARIABLE LUAJIT)
  # set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS
  #   "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -bundle_loader ${LUAJIT}"
  # )
endif()

if(WIN32)
  add_definitions(-DLUA_BUILD_AS_DLL -DLUA_LIB)
  target_link_libraries(luv uv lua51)
elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  target_link_libraries(luv uv rt)
else()
  target_link_libraries(luv uv)
endif()

if (LUV_INSTALL_PREFIX)
  install(TARGETS luv DESTINATION "${LUV_INSTALL_PREFIX}/lib")
endif(LUV_INSTALL_PREFIX)
