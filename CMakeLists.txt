cmake_minimum_required(VERSION 2.8)

project (luv)

include(deps/uv.cmake)
include(deps/luajit.cmake)

include_directories(deps/luajit/src)

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

