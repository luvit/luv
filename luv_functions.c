#ifndef LIB_LUV_FUNCTIONS
#define LIB_LUV_FUNCTIONS
#include "common.h"

static int new_tcp(lua_State* L) {
  uv_tcp_t* handle = luv_create_tcp(L);
  if (uv_tcp_init(uv_default_loop(), handle)) {
    luaL_error(L, "Problem initializing tcp handle %p", handle);
  }
  fprintf(stderr, "new tcp \tlhandle=%p handle=%p\n", handle->data, handle);
  return 1;
}

static int new_timer(lua_State* L) {
  uv_timer_t* handle = luv_create_timer(L);
  if (uv_timer_init(uv_default_loop(), handle)) {
    luaL_error(L, "Problem initializing timer handle %p", handle);
  }
  fprintf(stderr, "new timer \tlhandle=%p handle=%p\n", handle->data, handle);
  return 1;
}

static int luv_run_once(lua_State* L) {
  int ret = uv_run_once(uv_default_loop());
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_run(lua_State* L) {
  uv_run(uv_default_loop());
  return 0;
}

static const luaL_reg luv_functions[] = {
  {"newTcp", new_tcp},
  {"newTimer", new_timer},
  {"run", luv_run},
  {"runOnce", luv_run_once},
  {NULL, NULL}
};

#endif
