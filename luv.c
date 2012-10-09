#include <string.h>

#include "luv.h"
#include "luv_functions.c"
#include "luv_handle.c"
#include "luv_timer.c"
#include "luv_stream.c"
#include "luv_tcp.c"


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

  // Create userdata metatables
  luaL_newmetatable(L, "uv_handle");
  luaL_newmetatable(L, "uv_timer");
  luaL_newmetatable(L, "uv_stream");
  luaL_newmetatable(L, "uv_tcp");

  // uv_handle, uv_timer, uv_stream, uv_tcp

  // Create prototypes (method tables)
  lua_newtable(L);
  luv_setfuncs(L, luv_handle_m);
  lua_newtable(L);
  luv_setfuncs(L, luv_timer_m);
  lua_newtable(L);
  luv_setfuncs(L, luv_stream_m);
  lua_newtable(L);
  luv_setfuncs(L, luv_tcp_m);

  // uv_handle, uv_timer, uv_stream, uv_tcp
  // handle_m,  timer_m,  stream_m,  tcp_m

  // timer_m > handle_m
  lua_newtable(L); // timer_meta
  lua_pushvalue(L,  -5);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -4);

  // stream_m > handle_m
  lua_newtable(L); // stream_meta
  lua_pushvalue(L,  -5);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -3);

  // tcp_m > stream_m
  lua_newtable(L); // tcp_meta
  lua_pushvalue(L,  -3);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -2);

  lua_setfield(L, -5, "__index"); // tcp
  lua_pushcfunction(L, luv_newindex);
  lua_setfield(L, -5, "__newindex");

  lua_setfield(L, -5, "__index"); // stream

  lua_setfield(L, -5, "__index"); // timer
  lua_pushcfunction(L, luv_newindex);
  lua_setfield(L, -5, "__newindex");

  lua_setfield(L, -5, "__index"); // handle

  lua_pop(L, 4);


  // Module exports
  lua_newtable (L);
  luv_setfuncs(L, luv_functions);
  return 1;
}
