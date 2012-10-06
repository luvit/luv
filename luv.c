#include "luv.h"

#include "uv.h"

static int new_tcp(lua_State* L) {
  uv_tcp_t* handle = (uv_tcp_t*)lua_newuserdata(L, sizeof(uv_tcp_t));
  
  luaL_getmetatable(L, "uv_tcp");
  lua_setmetatable(L, -2);
  
  printf("%p\n", handle);
  return 1;
}

static void on_close(uv_handle_t* handle) {
  fprintf(stderr, "on_close \tlhandle=%p handle=%p\n", handle->data, handle);
}

static int luv_close(lua_State* L) {
  luaL_checktype(L, 1, LUA_TUSERDATA);
  uv_handle_t* handle = (uv_handle_t*)lua_touserdata(L, 1);
  fprintf(stderr, "close \tlhandle=%p handle=%p\n", handle->data, handle);

  if (uv_is_closing(handle)) {
    fprintf(stderr, "WARNING: Handle already closing \tlhandle=%p handle=%p\n", handle->data, handle);
    return 0;
  }

  uv_close(handle, on_close);
  return 0;
}

static const luaL_reg luv_f[] = {
  {"newTcp", new_tcp},
  {NULL, NULL}
};

static const luaL_reg handle_m[] = {
  {"close", luv_close},
  {NULL, NULL}
};

static const luaL_reg stream_m[] = {
/*  {"__index", vector_index}, */
  {NULL, NULL}
};

static const luaL_reg tcp_m[] = {
/*  {"__index", vector_index}, */
  {NULL, NULL}
};

LUALIB_API int luaopen_luv (lua_State *L) {

  lua_newtable(L); // handle methods table
  luaL_register(L, NULL, handle_m);
  lua_newtable(L); // stream methods table
  luaL_register(L, NULL, stream_m);
  lua_newtable(L); // tcp methods table
  luaL_register(L, NULL, tcp_m);


  luaL_newmetatable(L, "uv_tcp"); // Store tcp methods in tcp metatable
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "uv_stream"); // Store stream methods in stream metatable
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "uv_tcp"); // Store handle methods in handle metatable
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  // Module exports
  lua_newtable (L);
  luaL_register(L, NULL, luv_f);
  return 1;
}
