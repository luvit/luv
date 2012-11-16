#include "direct.h"

static int luv_loop_new(lua_State* L) {
  lua_pushlightuserdata(L, uv_loop_new());
  return 1;
}

static int luv_loop_delete(lua_State* L) {
  uv_loop_delete(lua_touserdata(L, 1));
  return 0;
}

static int luv_default_loop(lua_State* L) {
  lua_pushlightuserdata(L, uv_default_loop());
  return 1;
}

static int luv_run(lua_State* L) {
  lua_pushinteger(L, uv_run(lua_touserdata(L, 1)));
  return 1;
}

static int luv_run_once(lua_State* L) {
  lua_pushinteger(L, uv_run_once(lua_touserdata(L, 1)));
  return 1;
}

static int luv_ref(lua_State* L) {
  uv_ref(lua_touserdata(L, 1));
  return 0;
}

static int luv_unref(lua_State* L) {
  uv_unref(lua_touserdata(L, 1));
  return 0;
}

static int luv_update_time(lua_State* L) {
  uv_update_time(lua_touserdata(L, 1));
  return 0;
}

static int luv_now(lua_State* L) {
  lua_pushinteger(L, uv_now(lua_touserdata(L, 1)));
  return 1;
}

static const luaL_reg luv_functions[] = {
  {"loop_new", luv_loop_new},
  {"loop_delete", luv_loop_delete},
  {"default_loop", luv_default_loop},
  {"run", luv_run},
  {"run_once", luv_run_once},
  {"ref", luv_ref},
  {"unref", luv_unref},
  {"update_time", luv_update_time},
  {"now", luv_now},
  {NULL, NULL}
};

LUALIB_API int luaopen_direct (lua_State *L) {
  lua_newtable(L);
  luv_setfuncs(L, luv_functions);
  return 1;
}