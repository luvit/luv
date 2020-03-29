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

static uv_check_t* luv_check_check(lua_State* L, int index) {
  uv_check_t* handle = (uv_check_t*)luv_checkudata(L, index, "uv_check");
  luaL_argcheck(L, handle->type == UV_CHECK && handle->data, index, "Expected uv_check_t");
  return handle;
}

static int luv_new_check(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  uv_check_t* handle = (uv_check_t*)luv_newuserdata(L, sizeof(*handle));
  int ret = uv_check_init(ctx->loop, handle);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  handle->data = luv_setup_handle(L, ctx);
  return 1;
}

static void luv_check_cb(uv_check_t* handle) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  luv_call_callback(L, data, LUV_CHECK, 0);
}

static int luv_check_start(lua_State* L) {
  uv_check_t* handle = luv_check_check(L, 1);
  int ret;
  luv_check_callback(L, (luv_handle_t *)handle->data, LUV_CHECK, 2);
  ret = uv_check_start(handle, luv_check_cb);
  return luv_result(L, ret);
}

static int luv_check_stop(lua_State* L) {
  uv_check_t* handle = luv_check_check(L, 1);
  int ret = uv_check_stop(handle);
  return luv_result(L, ret);
}

