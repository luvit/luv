#include "common.h"

// lua 5.1.x and 5.2.x compatable way to mass set functions on an object
void luv_setfuncs(lua_State *L, const luaL_Reg *l) {
  for (; l->name != NULL; l++) {
    lua_pushcfunction(L, l->func);
    lua_setfield(L, -2, l->name);
  }
}

