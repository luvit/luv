#ifndef LIB_LUV_TIMER
#define LIB_LUV_TIMER
#include "common.h"

static void on_timeout(uv_timer_t* handle, int status) {
  lua_State* L = luv_prepare_event(handle->data);
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L) - 1;
#endif
  if (luv_get_callback(L, -1, "ontimeout")) {
    lua_call(L, 1, 0);
  }
  lua_pop(L, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
}

static int luv_timer_start(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_timer_t* handle = luv_get_timer(L, 1);
  int64_t timeout = luaL_checkinteger(L, 2);
  int64_t repeat = luaL_checkinteger(L, 3);
  if (uv_timer_start(handle, on_timeout, timeout, repeat)) {
    luaL_error(L, "Problem starting timer");
  }
  luv_handle_ref(L, handle->data, 1);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_timer_stop(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_timer_t* handle = luv_get_timer(L, 1);
  luv_handle_unref(L, handle->data);
  if (uv_timer_stop(handle)) {
    luaL_error(L, "Problem stopping timer");
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static const luaL_reg luv_timer_m[] = {
  {"start", luv_timer_start},
  {"stop", luv_timer_stop},
  {NULL, NULL}
};

#endif
