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
#include "lhandle.h"

// Given the ref to the userdata, create and return a new luv_handle struct
static luv_handle_t* luv_create_handle(int ref) {
  luv_handle_t* data = malloc(sizeof(*data));
  data->ref = ref;
  data->callbacks[0] = LUA_NOREF;
  data->callbacks[1] = LUA_NOREF;
  return data;
}

static luv_handle_t* luv_setup_handle(lua_State* L) {
  luaL_checktype(L, -1, LUA_TUSERDATA);

  luaL_getmetatable(L, "uv_handle");
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  return luv_create_handle(luaL_ref(L, LUA_REGISTRYINDEX));
}

static void luv_check_callback(lua_State* L, luv_handle_t* data, luv_callback_id id, int index) {
  luaL_checktype(L, index, LUA_TFUNCTION);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callbacks[id]);
  lua_pushvalue(L, index);
  data->callbacks[id] = luaL_ref(L, LUA_REGISTRYINDEX);
}

static void luv_call_callback(lua_State* L, luv_handle_t* data, luv_callback_id id, int nargs) {
  int ref = data->callbacks[id];
  if (ref == LUA_NOREF) {
    lua_pop(L, nargs);
  }
  else {
    // Get the callback
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    // And insert it before the args if there are any.
    if (nargs) {
      lua_insert(L, -1 - nargs);
    }
    lua_call(L, nargs, 0);
  }
}

static void luv_cleanup_handle(lua_State* L, luv_handle_t* data) {
  luaL_unref(L, LUA_REGISTRYINDEX, data->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callbacks[0]);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callbacks[1]);
  free(data);
}

static void luv_find_handle(lua_State* L, luv_handle_t* data) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, data->ref);
}
