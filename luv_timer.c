#ifndef LIB_LUV_TIMER
#define LIB_LUV_TIMER
#include "common.h"

static void on_timeout(uv_timer_t* handle, int status) {
  lua_State* L = luv_prepare_event(handle->data);

  /* Get the timeout handler */
  lua_getfenv(L, -1);
  lua_getfield(L, -1, "ontimeout");
  lua_remove(L, -2);
  
  /* call it */
  if (lua_isfunction(L, -1)) {
    lua_pushvalue(L, -2); // set self to timer
    lua_call(L, 1, 0);
  } else {
    lua_pop(L, 1);
  }
}

static int luv_timer_start(lua_State* L) {
  uv_timer_t* handle = luv_get_timer(L, 1);
  int64_t timeout = luaL_checkinteger(L, 2);
  int64_t repeat = luaL_checkinteger(L, 3);
  if (uv_timer_start(handle, on_timeout, timeout, repeat)) {
    luaL_error(L, "Problem starting timer");
  }
  luv_handle_ref(L, handle->data, 1);
  return 0;
}

static int luv_timer_stop(lua_State* L) {
  uv_timer_t* handle = luv_get_timer(L, 1);
  luv_handle_unref(L, handle->data);
  if (uv_timer_stop(handle)) {
    luaL_error(L, "Problem stopping timer");
  }
  return 0;
}

static const luaL_reg luv_timer_m[] = {
  {"start", luv_timer_start},
  {"stop", luv_timer_stop},
  {NULL, NULL}
};

#endif
