cmake_minimum_required(VERSION 2.8)

project (luv)

set(CMAKE_BUILD_TYPE Release)

include(uv.cmake)

include_directories(
  /usr/local/include/luajit-2.0
  /usr/include/luajit-2.0
)

add_library (luv SHARED src/luv.c)

target_link_libraries(luv uv)
