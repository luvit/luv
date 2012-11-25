#ifndef LIB_LUV_FUNCTIONS
#define LIB_LUV_FUNCTIONS
#include "common.h"

static int new_tcp(lua_State* L) {
  uv_tcp_t* handle = luv_create_tcp(L);
  if (uv_tcp_init(uv_default_loop(), handle)) {
    luaL_error(L, "Problem initializing tcp handle %p", handle);
  }
//  fprintf(stderr, "new tcp \tlhandle=%p handle=%p\n", handle->data, handle);
  return 1;
}

static int new_timer(lua_State* L) {
  uv_timer_t* handle = luv_create_timer(L);
  if (uv_timer_init(uv_default_loop(), handle)) {
    luaL_error(L, "Problem initializing timer handle %p", handle);
  }
//  fprintf(stderr, "new timer \tlhandle=%p handle=%p\n", handle->data, handle);
  return 1;
}

static int new_tty(lua_State* L) {
  uv_tty_t* handle = luv_create_tty(L);
  uv_file fd = luaL_checkint(L, 1);
  int readable = lua_toboolean(L, 2);
  if (uv_tty_init(uv_default_loop(), handle, fd, readable)) {
    luaL_error(L, "Problem initializing tty handle %p", handle);
  }
//  fprintf(stderr, "new timer \tlhandle=%p handle=%p\n", handle->data, handle);
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

static int luv_guess_handle(lua_State* L) {
  uv_file file = luaL_checkint(L, 1);
  switch (uv_guess_handle(file)) {
#define XX(uc, lc) case UV_##uc: lua_pushstring(L, #uc); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    case UV_FILE: lua_pushstring(L, "FILE"); break;
    default: lua_pushstring(L, "UNKNOWN"); break;
  }
  return 1;
}

static int luv_ref(lua_State* L) {
  uv_ref(luv_get_handle(L, 1));
  return 0;
}

static int luv_unref(lua_State* L) {
  uv_unref(luv_get_handle(L, 1));
  return 0;
}

static int luv_update_time(lua_State* L) {
  uv_update_time(uv_default_loop());
  return 0;
}

static int luv_now(lua_State* L) {
  lua_pushnumber(L, uv_now(uv_default_loop()));
  return 1;
}

static const luaL_reg luv_functions[] = {
  {"new_tcp", new_tcp},
  {"new_timer", new_timer},
  {"new_tty", new_tty},
  {"run", luv_run},
  {"run_once", luv_run_once},
  {"guess_handle", luv_guess_handle},
  {"ref", luv_ref},
  {"unref", luv_unref},
  {"update_time", luv_update_time},
  {"now", luv_now},
  {NULL, NULL}
};

#endif
