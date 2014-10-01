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
#include "lreq.h"

// Store a lua callback in a luv_req for the continuation.
// The uv_req_t is assumed to be at the top of the stack
static luv_req_t* luv_setup_req(lua_State* L, int fn_index) {
  luv_req_t* data;

  luaL_checktype(L, -1, LUA_TUSERDATA);
  luaL_checktype(L, fn_index, LUA_TFUNCTION);

  luaL_getmetatable(L, "uv_req");
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  data = malloc(sizeof(*data));
  data->req_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_pushvalue(L, fn_index);
  data->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  return data;
}
