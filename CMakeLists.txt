cmake_minimum_required(VERSION 2.8)

project (luv)

set(CMAKE_BUILD_TYPE Release)

include(uv.cmake)

include_directories(
  /usr/local/include/luajit-2.0
  /usr/include/luajit-2.0
)

add_library (luv MODULE src/luv.c $<TARGET_OBJECTS:uv>)

set_target_properties(luv PROPERTIES OUTPUT_NAME "luv")
set_target_properties(luv PROPERTIES PREFIX "")



if("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
  target_link_libraries(luv
    pthread
    kvm
  )
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  target_link_libraries(luv
    pthread
  )
endif()

if(WIN32)
  target_link_libraries(luv
    ws2_32.lib
    shell32.lib
    psapi.lib
    iphlpapi.lib
    advapi32.lib
  )
endif()

if(APPLE)
  find_library(FOUNDATION_LIBRARY Foundation)
  find_library(CORESERVICES_LIBRARY CoreServices)
  find_library(APPLICATION_SERVICES_LIBRARY ApplicationServices)
  target_link_libraries(luv
    ${FOUNDATION_LIBRARY}
    ${CORESERVICES_LIBRARY}
    ${APPLICATION_SERVICES_LIBRARY}
  )
endif()
