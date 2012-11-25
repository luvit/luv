#include <string.h>

#include "luv.h"
#include "luv_functions.c"

static int luv_newindex(lua_State* L) {
  lua_getfenv(L, 1);
  lua_pushvalue(L, 2);
  lua_pushvalue(L, 3);
  lua_rawset(L, -3);
  lua_pop(L, 1);
  return 0;
}

static int luv_index(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  lua_getfenv(L, 1);
  lua_pushvalue(L, 2);
  lua_rawget(L, -2);
  lua_remove(L, -2);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}

LUALIB_API int luaopen_luv (lua_State *L) {

  luv_main_thread = L;

  luaL_newmetatable(L, "luv_handle");
  lua_pushcfunction(L, luv_newindex);
  lua_setfield(L, -2, "__newindex");
  lua_pushcfunction(L, luv_index);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  // Module exports
  lua_newtable (L);
  luv_setfuncs(L, luv_functions);
  return 1;
}
