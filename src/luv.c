/*
 *  Copyright 2014 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#include <string.h>

#include "luv.h"
#include "luv_functions.c"

static int luv_newindex(lua_State* L) {
  lua_getuservalue(L, 1);
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

  /* Get handle type if requested */
  const char* key = lua_tostring(L, 2);
  if (strcmp(key, "type") == 0) {
    luv_handle_t* lhandle = (luv_handle_t*)luaL_checkudata(L, 1, "luv_handle");
    switch (lhandle->handle->type) {
#define XX(uc, lc) case UV_##uc: lua_pushstring(L, #uc); break;
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
      default: lua_pushstring(L, "UNKNOWN"); break;
    }
    return 1;
  }

  lua_getuservalue(L, 1);
  lua_pushvalue(L, 2);
  lua_rawget(L, -2);
  lua_remove(L, -2);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 1);
#endif
  return 1;
}

static int luv_tostring(lua_State* L) {
  luv_handle_t* lhandle = (luv_handle_t*)luaL_checkudata(L, 1, "luv_handle");
  switch (lhandle->handle->type) {
#define XX(uc, lc) case UV_##uc: lua_pushfstring(L, "uv_%s_t: %p", #lc, lhandle->handle); break;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default: lua_pushfstring(L, "userdata: %p", lhandle->handle); break;
  }
  return 1;
}


LUALIB_API int luaopen_luv (lua_State *L) {

  luv_main_thread = L;

  luaL_newmetatable(L, "luv_handle");
  lua_pushcfunction(L, luv_newindex);
  lua_setfield(L, -2, "__newindex");
  lua_pushcfunction(L, luv_index);
  lua_setfield(L, -2, "__index");
  lua_pushcfunction(L, luv_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pop(L, 1);

  /* Module exports */
  luaL_newlib(L, luv_functions);
  return 1;
}
