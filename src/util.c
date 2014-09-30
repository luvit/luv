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

// Create a weak table for mapping pointers to userdata.
static void util_init(lua_State* L) {
  // Create a table for storing active coroutines.
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "active_threads");

  lua_atpanic(L, on_panic);
}

static int on_panic(lua_State* L) {
  luv_stack_dump(L, "PANIC");
  return 0;
}

static void luv_stack_dump(lua_State* L, const char* name) {
  int i, l;
  printf("\nAPI STACK DUMP %p %d: %s\n", L, lua_status(L), name);
  for (i = 1, l = lua_gettop(L); i <= l; i++) {
    int type = lua_type(L, i);
    switch (type) {
      case LUA_TSTRING:
        printf("  %d %s \"%s\"\n", i, lua_typename(L, type), lua_tostring(L, i));
        break;
      default:
        printf("  %d %s\n", i, lua_typename(L, type));
        break;
    }
  }
  assert(l == lua_gettop(L));
}

static int luv_error(lua_State* L, int ret) {
  lua_pushnil(L);
  // For now log errors to stderr in case they aren't asserted or checked for.
  fprintf(stderr, "%s: %s\n", uv_err_name(ret), uv_strerror(ret));
  lua_pushstring(L, uv_err_name(ret));
  return 2;
}

static void luv_ref_state(luv_ref_t* data, lua_State* L) {
  lua_pushthread(L);
  luaL_unref(L, LUA_REGISTRYINDEX, data->lref);
  data->L = L;
  data->lref = luaL_ref(L, LUA_REGISTRYINDEX);
}

static void luv_unref_state(luv_ref_t* data) {
  if (data->lref == LUA_NOREF) return;
  luaL_unref(data->L, LUA_REGISTRYINDEX, data->lref);
  data->lref = LUA_NOREF;
}

static void luv_ccall(lua_State* L, luv_ref_t* ref, int nargs) {
  int top = lua_gettop(L);
  int ret;
  lua_State* T = lua_newthread(L);
  if (ref) ref->L = T;
  lua_insert(L, -2 - nargs); // Push coroutine above function and userdata
  lua_xmove(L, T, nargs + 1);
  ret = lua_resume(T, L, nargs);
  lua_pop(L, 1);
  if (ret && ret != LUA_YIELD) {
    on_panic(T);
  }
  assert(lua_gettop(L) == top - nargs - 1);
}

static void luv_emit_event(lua_State* L, luv_ref_t* ref, const char* name, int nargs) {
  lua_getuservalue(L, -nargs);
  lua_getfield(L, -1, name);
  if (lua_type(L, -1) != LUA_TFUNCTION) {
    lua_pop(L, 2 + nargs);
    return;
  }
  lua_remove(L, -2); // Remove uservalue
  lua_insert(L, -1 - nargs); // Push function above userdata
  luv_ccall(L, ref, nargs);
}

static int luv_wait(lua_State* L, luv_ref_t* data, int status) {
  if (status < 0) return luv_error(L, status);

  assert(data);
  luv_ref_state(data, L);

  // Store the current thread in active_threads
  lua_getfield(L, LUA_REGISTRYINDEX, "active_threads");
  lua_pushthread(L);
  lua_pushboolean(L, 1);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  return lua_yield(L, 0);
}

static void luv_resume(lua_State* L, int nargs) {
  int ret;

  // Remove the current thread from active_threads
  lua_getfield(L, LUA_REGISTRYINDEX, "active_threads");
  lua_pushthread(L);
  lua_pushnil(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  ret = lua_resume(L, NULL, nargs);
  if (ret && ret != LUA_YIELD) {
    on_panic(L);
  }
}

static void luv_resume_with_status(lua_State* L, int status, int nargs) {
  lua_getglobal(L, "_ref");
  luaL_unref(L, LUA_REGISTRYINDEX, lua_tointeger(L, -1));
  lua_pushnil(L);
  lua_setglobal(L, "_ref");
  if (status < 0) {
    fprintf(stderr, "%s: %s\n", uv_err_name(status), uv_strerror(status));
    lua_pushstring(L, uv_err_name(status));
  }
  else {
    lua_pushnil(L);
  }
  if (nargs) {
    lua_insert(L, -1 - nargs);
  }
  return luv_resume(L, 1 + nargs);
}
