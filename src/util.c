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
#include "luv.h"

void luv_stack_dump(lua_State* L, const char* name) {
  int i, l;
  fprintf(stderr, "\nAPI STACK DUMP %p %d: %s\n", L, lua_status(L), name);
  for (i = 1, l = lua_gettop(L); i <= l; i++) {
    int type = lua_type(L, i);
    switch (type) {
      case LUA_TSTRING:
        fprintf(stderr, "  %d %s \"%s\"\n", i, lua_typename(L, type), lua_tostring(L, i));
        break;
      case LUA_TNUMBER:
        fprintf(stderr, "  %d %s %ld\n", i, lua_typename(L, type), lua_tointeger(L, i));
        break;
      case LUA_TUSERDATA:
        fprintf(stderr, "  %d %s %p\n", i, lua_typename(L, type), lua_touserdata(L, i));
        break;
      default:
        fprintf(stderr, "  %d %s\n", i, lua_typename(L, type));
        break;
    }
  }
  assert(l == lua_gettop(L));
}

static int luv_error(lua_State* L, int status) {
  lua_pushnil(L);
  // For now log errors to stderr in case they aren't asserted or checked for.
  fprintf(stderr, "%s: %s\n", uv_err_name(status), uv_strerror(status));
  lua_pushfstring(L, "%s: %s", uv_err_name(status), uv_strerror(status));
  lua_pushstring(L, uv_err_name(status));
  lua_pushnumber(L, status);
  return 4;
}

static void luv_status(lua_State* L, int status) {
  if (status < 0) {
    // For now log errors to stderr in case they aren't asserted or checked for.
    fprintf(stderr, "%s: %s\n", uv_err_name(status), uv_strerror(status));
    lua_pushstring(L, uv_err_name(status));
  }
  else {
    lua_pushnil(L);
  }
}
