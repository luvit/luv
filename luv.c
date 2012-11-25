#include <string.h>

#include "luv.h"
#include "luv_functions.c"

int luv_newindex(lua_State* L) {
  lua_getfenv(L, 1);
  lua_pushvalue(L, 2);
  lua_pushvalue(L, 3);
  lua_rawset(L, -3);
  lua_pop(L, 1);
  return 0;
}

LUALIB_API int luaopen_luv (lua_State *L) {

  luv_main_thread = L;

  luaL_newmetatable(L, "luv_handle");
  lua_pushcfunction(L, luv_newindex);
  lua_setfield(L, -2, "__newindex");
  lua_pop(L, 1);

  // Module exports
  lua_newtable (L);
  luv_setfuncs(L, luv_functions);
  return 1;
}
