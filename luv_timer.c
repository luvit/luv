#ifndef LIB_LUV_TIMER
#define LIB_LUV_TIMER
#include "common.h"

static int luv_start(lua_State* L) {
  return 0;
}

static const luaL_reg luv_timer_m[] = {
  {"start", luv_start},
  {NULL, NULL}
};

#endif
