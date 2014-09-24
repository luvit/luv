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

static void luv_stack_dump(lua_State* L, const char* name) {
  int i, l;
  printf("\nAPI STACK DUMP %p: %s\n", L, name);
  for (i = 1, l = lua_gettop(L); i <= l; i++) {
    lua_getglobal(L, "tostring");
    lua_pushvalue(L, i);
    lua_call(L, 1, 1);
    printf("  %d %s\n", i, lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  assert(l == lua_gettop(L));
}

static int on_panic(lua_State* L) {
  luv_stack_dump(L, "PANIC");
  return 0;
}

// Create a weak table for mapping pointers to userdata.
static void util_init(lua_State* L) {
  lua_newtable(L);
  lua_newtable(L);
  lua_pushstring(L, "kv");
  lua_setfield(L, -2, "__mode");
  lua_setmetatable(L, -2);
  lua_setfield(L, LUA_REGISTRYINDEX, "udata_map");
  lua_atpanic(L, on_panic);
}

static int luv_error(lua_State* L, int ret) {
  lua_pushnil(L);
  // For now log errors to stderr in case they aren't asserted or checked for.
  fprintf(stderr, "%s: %s\n", uv_err_name(ret), uv_strerror(ret));
  lua_pushstring(L, uv_err_name(ret));
  return 2;
}

static void luv_ccall(lua_State* L, int nargs) {
  int top = lua_gettop(L);
  int ret;
  lua_State* T = lua_newthread(L);
  lua_insert(L, -2 - nargs); // Push coroutine above function and userdata
  lua_xmove(L, T, nargs + 1);
  ret = lua_resume(T, L, nargs);
  lua_pop(L, 1);
  if (ret && ret != LUA_YIELD) {
    on_panic(T);
  }
  assert(lua_gettop(L) == top - nargs - 1);
}

static void luv_emit_event(lua_State* L, const char* name, int nargs) {
  lua_getuservalue(L, -nargs);
  lua_getfield(L, -1, name);
  if (lua_type(L, -1) != LUA_TFUNCTION) {
    lua_pop(L, 2 + nargs);
    return;
  }
  lua_remove(L, -2); // Remove uservalue
  lua_insert(L, -1 - nargs); // Push function above userdata
  luv_ccall(L, nargs);
}


static void handle_unref(lua_State* L, int index) {
  int ref;
  lua_getuservalue(L, index);
  lua_getfield(L, -1, "ref");
  ref = lua_tointeger(L, -1);
  if (!ref) {
    lua_pop(L, 2);
    return;
  }
  lua_pop(L, 1);
  luaL_unref(L, LUA_REGISTRYINDEX, ref);
  lua_pushnil(L);
  lua_setfield(L, -2, "ref");
  lua_pop(L, 1);
}

// Given a userdata on top of the stack, give it a metatable and an environment.
// +0 to stack
static void setup_udata(lua_State* L, uv_handle_t* handle, const char* type) {
  handle->data = L;
  // Tag with the given metatable type
  luaL_getmetatable(L, type);
  lua_setmetatable(L, -2);
  // Create a new environment for storing stuff
  lua_newtable(L);

  // Store a ref to self in the environment
  lua_pushvalue(L, -2);
  lua_pushinteger(L, luaL_ref(L, LUA_REGISTRYINDEX));
  lua_setfield(L, -2, "ref");

  // Store the new environment as the uservalue.
  lua_setuservalue(L, -2);
  // Record in the registry so we can match pointers to it later.
  lua_getfield(L, LUA_REGISTRYINDEX, "udata_map");
  lua_pushlightuserdata(L, handle); // push raw as lightudata
  lua_pushvalue(L, -3);             // push copy of udata
  lua_rawset(L, -3);                // udata_map[raw]=udata
  lua_pop(L, 1);                    // pop udata_map
}

// Given a pointer, push the corresponding userdata on the stack (or nil)
// +1 to stack [udata]
static void find_udata(lua_State* L, void* handle) {
  lua_getfield(L, LUA_REGISTRYINDEX, "udata_map");
  lua_pushlightuserdata(L, handle); // push pointer
  lua_rawget(L, -2);                // replace with userdata
  lua_remove(L, -2);                // Remote udata_map
}

