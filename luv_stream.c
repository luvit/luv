#ifndef LIB_LUV_STREAM
#define LIB_LUV_STREAM
#include "common.h"

static int luv_write(lua_State* L) {
  luaL_error(L, "TODO: Implement luv_write");
  return 0;
}

static int luv_shutdown(lua_State* L) {
  luaL_error(L, "TODO: Implement luv_shutdown");
  return 0;
}

static int luv_read_start(lua_State* L) {
  luaL_error(L, "TODO: Implement luv_read_start");
  return 0;
}

static int luv_read_stop(lua_State* L) {
  luaL_error(L, "TODO: Implement luv_read_stop");
  return 0;
}

static int luv_listen(lua_State* L) {
  luaL_error(L, "TODO: Implement luv_listen");
  return 0;
}

static int luv_accept(lua_State* L) {
  luaL_error(L, "TODO: Implement luv_accept");
  return 0;
}

static const luaL_reg luv_stream_m[] = {
  {"write", luv_write},
  {"shutdown", luv_shutdown},
  {"readStart", luv_read_start},
  {"readStop", luv_read_stop},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {NULL, NULL}
};

#endif

