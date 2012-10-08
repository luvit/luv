#ifndef LIB_LUV_COMMON
#define LIB_LUV_COMMON

#include <lua.h>
#include <lauxlib.h>
#include "uv.h"

void luv_setfuncs(lua_State *L, const luaL_Reg *l);

#endif
