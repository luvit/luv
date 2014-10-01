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
  data->callbacks = NULL;
  return data;
}

// Sets a handler callback in the linked list.
// If this ID already exists, replaces it and returns the old ref.
// If it's new, appends and returns LUA_NOREF
static int luv_set_callback(luv_handle_t* data, luv_callback_id id, int ref) {
  luv_callback_t* current = data->callbacks;

  // Special case for first handler
  if (!current) {
    current = data->callbacks = malloc(sizeof(*current));
    current->id = id;
    current->ref = ref;
    current->next = NULL;
    return LUA_NOREF;
  }

  while (1) {
    // If the handler already exists, replace the callback.
    if (current->id == id) {
      int old = current->ref;
      current->ref = ref;
      return old;
    }

    // If the end is reached, append a new node
    if (!current->next) {
      current = current->next = malloc(sizeof(*current->next));
      current->id = id;
      current->ref = ref;
      current->next = NULL;
      return LUA_NOREF;
    }

    // Walk the list
    current = current->next;
  }
}

static int luv_find_callback(luv_handle_t* data, luv_callback_id id) {
  luv_callback_t* current = data->callbacks;

  while (1) {
    if (!current) return LUA_NOREF;
    if (current->id == id) return current->ref;
    current = current->next;
  }
}

static luv_handle_t* luv_setup_handle(lua_State* L) {
  luaL_checktype(L, -1, LUA_TUSERDATA);

  luaL_getmetatable(L, "uv_handle");
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  return luv_create_handle(
    luaL_ref(L, LUA_REGISTRYINDEX));
}

static void luv_check_callback(lua_State* L, luv_handle_t* data, luv_callback_id id, int index) {
  luaL_checktype(L, index, LUA_TFUNCTION);
  lua_pushvalue(L, index);
  luaL_unref(L, LUA_REGISTRYINDEX,
    luv_set_callback(data, id,
      luaL_ref(L, LUA_REGISTRYINDEX)));
}

static void luv_call_callback(lua_State* L, luv_handle_t* data, luv_callback_id id, int nargs) {
  int ref = luv_find_callback(data, id);
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

static void luv_free_handlers(lua_State* L, luv_callback_t* data) {
  if (data->next) luv_free_handlers(L, data->next);
  luaL_unref(L, LUA_REGISTRYINDEX, data->ref);
  free(data);
}

static void luv_cleanup_handle(lua_State* L, luv_handle_t* data) {
  if (data->callbacks) luv_free_handlers(L, data->callbacks);
  luaL_unref(L, LUA_REGISTRYINDEX, data->ref);
  free(data);
}

static void luv_find_handle(lua_State* L, luv_handle_t* data) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, data->ref);
}
