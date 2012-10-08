#ifndef LIB_LUV_TCP
#define LIB_LUV_TCP
#include "common.h"

static int luv_bind(lua_State* L) {
  luaL_error(L, "TODO: Implement luv_shutdown");
  return 0;
}

static const luaL_reg luv_tcp_m[] = {
  {"bind", luv_bind},
  {NULL, NULL}
};

#endif
