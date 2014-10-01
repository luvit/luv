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

static uv_async_t* luv_check_async(lua_State* L, int index) {
  uv_async_t* handle = luaL_checkudata(L, index, "uv_handle");
  luaL_argcheck(L, handle->type = UV_ASYNC, index, "Expected uv_async_t");
  return handle;
}

static void luv_async_cb(uv_async_t* handle) {
  luv_handle_t* data = handle->data;
  luv_find_handle(R, data);
  luv_call_callback(R, data, LUV_ASYNC, 1);
}

static int luv_new_async(lua_State* L) {
  uv_async_t* handle;
  int ret;
  luaL_checktype(L, 1, LUA_TFUNCTION);
  handle = lua_newuserdata(L, sizeof(*handle));
  ret = uv_async_init(uv_default_loop(), handle, luv_async_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  handle->data = luv_setup_handle(L);
  luv_check_callback(L, handle->data, LUV_ASYNC, 1);
  return 1;
}

static int luv_async_send(lua_State* L) {
  uv_async_t* handle = luv_check_async(L, 1);
  int ret = uv_async_send(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}
