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

static uv_async_t* luv_check_async(lua_State* L, int index) {
  uv_async_t* handle = (uv_async_t*)luv_checkudata(L, index, "uv_async");
  luaL_argcheck(L, handle->type == UV_ASYNC && handle->data, index, "Expected uv_async_t");
  return handle;
}

static void luv_async_cb(uv_async_t* handle) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  int n = luv_thread_arg_push(L, (luv_thread_arg_t*)data->extra, LUVF_THREAD_SIDE_MAIN);
  luv_call_callback(L, data, LUV_ASYNC, n);
  luv_thread_arg_clear(L, (luv_thread_arg_t*)data->extra, LUVF_THREAD_SIDE_MAIN);
}

static int luv_new_async(lua_State* L) {
  uv_async_t* handle;
  luv_handle_t* data;
  int ret;
  luv_ctx_t* ctx = luv_context(L);
  luaL_checktype(L, 1, LUA_TFUNCTION);
  handle = (uv_async_t*)luv_newuserdata(L, sizeof(*handle));
  ret = uv_async_init(ctx->loop, handle, luv_async_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  data = luv_setup_handle(L, ctx);
  data->extra = (luv_thread_arg_t*)malloc(sizeof(luv_thread_arg_t));
  data->extra_gc = free;
  memset(data->extra, 0, sizeof(luv_thread_arg_t));
  handle->data = data;
  luv_check_callback(L, (luv_handle_t*)handle->data, LUV_ASYNC, 1);
  return 1;
}

static int luv_async_send(lua_State* L) {
  int ret;
  uv_async_t* handle = luv_check_async(L, 1);
  luv_thread_arg_t* arg = (luv_thread_arg_t *)((luv_handle_t*) handle->data)->extra;

  luv_thread_arg_set(L, arg, 2, lua_gettop(L), LUVF_THREAD_MODE_ASYNC|LUVF_THREAD_SIDE_CHILD);
  ret = uv_async_send(handle);
  luv_thread_arg_clear(L, arg, LUVF_THREAD_SIDE_CHILD);
  return luv_result(L, ret);
}
