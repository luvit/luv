cmake_minimum_required(VERSION 2.8)

project(luv)

# general info
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

# configurable options
set(USE_STRICT OFF)

option(USE_STRICT "Treat warning as errors" ${USE_STRICT})
## TODO: include config.h into luv.h
#configure_file(cmake/config.h.in config.h)

# setup compiler flags
add_definitions(-fPIC)
if(USE_STRICT)
  add_definitions(-Wall -Werror)
endif()

# collect source files
list(APPEND SOURCES
  luv.c common.c luv_functions.c luv_handle.c luv_stream.c
  luv_tcp.c luv_timer.c
)

# find lua/luajit
include(cmake/FindLua.cmake)
include_directories(${LUA_INCLUDE_DIR})
list(APPEND LIBS ${LUA_LIBRARIES})

# setup linker flags
find_package(Threads REQUIRED)
list(APPEND LIBS ${CMAKE_THREAD_LIBS_INIT})

# build libuv
execute_process(WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  COMMAND git submodule update --init libuv)
file(COPY "${PROJECT_SOURCE_DIR}/cmake/uv/CMakeLists.txt"
  DESTINATION "${PROJECT_SOURCE_DIR}/libuv")
add_subdirectory("${PROJECT_SOURCE_DIR}/libuv")
include_directories("${PROJECT_SOURCE_DIR}/libuv/include")
list(APPEND LIBS uv rt)

# build target module
add_library(luv SHARED ${SOURCES})
target_link_libraries(luv ${LIBS})
set_target_properties(luv PROPERTIES PREFIX "")

# install
install(TARGETS luv LIBRARY DESTINATION "${INSTALL_CMOD}")
