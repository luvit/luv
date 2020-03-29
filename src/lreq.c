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


static int luv_check_continuation(lua_State* L, int index) {
  if (lua_isnoneornil(L, index)) return LUA_NOREF;
  luv_check_callable(L, index);
  lua_pushvalue(L, index);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}

// Store a lua callback in a luv_req for the continuation.
// The uv_req_t is assumed to be at the top of the stack
static luv_req_t* luv_setup_req(lua_State* L, luv_ctx_t* ctx, int cb_ref) {
  luv_req_t* data;

  luaL_checktype(L, -1, LUA_TUSERDATA);

  data = (luv_req_t*)malloc(sizeof(*data));
  if (!data) luaL_error(L, "Problem allocating luv request");

  luaL_getmetatable(L, "uv_req");
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  data->req_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  data->callback_ref = cb_ref;
  data->data_ref = LUA_NOREF;
  data->ctx = ctx;
  data->data = NULL;

  return data;
}


static void luv_fulfill_req(lua_State* L, luv_req_t* data, int nargs) {
  if (data->callback_ref == LUA_NOREF) {
    lua_pop(L, nargs);
  }
  else {
    // Get the callback
    lua_rawgeti(L, LUA_REGISTRYINDEX, data->callback_ref);
    // And insert it before the args if there are any.
    if (nargs) {
      lua_insert(L, -1 - nargs);
    }
    data->ctx->pcall(L, nargs, 0, 0);
  }
}

static void luv_cleanup_req(lua_State* L, luv_req_t* data) {
  int i;
  luaL_unref(L, LUA_REGISTRYINDEX, data->req_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callback_ref);
  if (data->data_ref == LUV_REQ_MULTIREF) {
    for (i = 0; ((int*)(data->data))[i] != LUA_NOREF; i++) {
      luaL_unref(L, LUA_REGISTRYINDEX, ((int*)(data->data))[i]);
    }
  }
  else
    luaL_unref(L, LUA_REGISTRYINDEX, data->data_ref);
  free(data->data);
  free(data);
}
