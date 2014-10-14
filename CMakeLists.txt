cmake_minimum_required(VERSION 2.8)

project (luv)

set(CMAKE_BUILD_TYPE Release)

include(uv.cmake)

include_directories(
  /usr/local/include/luajit-2.0
  /usr/include/luajit-2.0
)

add_library (luv MODULE src/luv.c)

set_target_properties(luv PROPERTIES OUTPUT_NAME "luv")
set_target_properties(luv PROPERTIES PREFIX "")

target_link_libraries(luv uv)
