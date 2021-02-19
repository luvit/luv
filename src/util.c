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
#include "private.h"

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
        fprintf(stderr, "  %d %s %ld\n", i, lua_typename(L, type), (long int) lua_tointeger(L, i));
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
  assert(status < 0);
  lua_pushnil(L);
  lua_pushfstring(L, "%s: %s", uv_err_name(status), uv_strerror(status));
  lua_pushstring(L, uv_err_name(status));
  return 3;
}

static int luv_result(lua_State* L, int status) {
  if (status < 0) return luv_error(L, status);
  lua_pushinteger(L, status);
  return 1;
}

static void luv_status(lua_State* L, int status) {
  if (status < 0) {
    lua_pushstring(L, uv_err_name(status));
  }
  else {
    lua_pushnil(L);
  }
}

static int luv_is_callable(lua_State* L, int index) {
  if (luaL_getmetafield(L, index, "__call") != LUA_TNIL) {
    // getmetatable(x).__call must be a function for x() to work
    int callable = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return callable;
  }
  return lua_isfunction(L, index);
}

static void luv_check_callable(lua_State* L, int index) {
  if (luv_is_callable(L, index))
    return;
  else
    luv_arg_type_error(L, index, "function or callable table expected, got %s");
}

static int luv_arg_type_error(lua_State* L, int index, const char* fmt) {
  const char *msg;
  const char *typearg;  /* name for the type of the actual argument */
  if (luaL_getmetafield(L, index, "__name") == LUA_TSTRING)
    typearg = lua_tostring(L, -1);  /* use the given type name */
  else if (lua_type(L, index) == LUA_TLIGHTUSERDATA)
    typearg = "light userdata";  /* special name for messages */
  else
    typearg = luaL_typename(L, index);  /* standard name */
  msg = lua_pushfstring(L, fmt, typearg);
  return luaL_argerror(L, index, msg);
}

#if LUV_UV_VERSION_GEQ(1, 10, 0)
static int luv_translate_sys_error(lua_State* L) {
  int status = luaL_checkinteger(L, 1);
  status = uv_translate_sys_error(status);
  if (status < 0) {
    luv_error(L, status);
    lua_remove(L, -3);
    return 2;
  }
  return 0;
}
#endif

static int luv_optboolean(lua_State*L, int idx, int val) {
  idx = lua_absindex(L, idx);
  luaL_argcheck(L, lua_isboolean(L, idx) || lua_isnoneornil(L, idx), idx, "Expected boolean or nil");

  if (lua_isboolean(L, idx))
    val = lua_toboolean(L, idx);
  return val;
}
