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

static uv_signal_t* luv_check_signal(lua_State* L, int index) {
  uv_signal_t* handle = (uv_signal_t*)luv_checkudata(L, index, "uv_signal");
  luaL_argcheck(L, handle->type == UV_SIGNAL && handle->data, index, "Expected uv_signal_t");
  return handle;
}

static int luv_new_signal(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  uv_signal_t* handle = (uv_signal_t*)luv_newuserdata(L, sizeof(*handle));
  int ret = uv_signal_init(ctx->loop, handle);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  handle->data = luv_setup_handle(L, ctx);
  return 1;
}

static void luv_signal_cb(uv_signal_t* handle, int signum) {
  luv_handle_t* data = (luv_handle_t*)handle->data;
  lua_State* L = data->ctx->L;
  lua_pushstring(L, luv_sig_num_to_string(signum));
  luv_call_callback(L, data, LUV_SIGNAL, 1);
}

static int luv_signal_start(lua_State* L) {
  uv_signal_t* handle = luv_check_signal(L, 1);
  int signum, ret;
  if (lua_isnumber(L, 2)) {
    signum = lua_tointeger(L, 2);
  }
  else if (lua_isstring(L, 2)) {
    signum = luv_sig_string_to_num(luaL_checkstring(L, 2));
    luaL_argcheck(L, signum, 2, "Invalid Signal name");
  }
  else {
    return luaL_argerror(L, 2, "Missing Signal name");
  }

  if (!lua_isnoneornil(L, 3)) {
    luv_check_callback(L, (luv_handle_t*)handle->data, LUV_SIGNAL, 3);
  }
  ret = uv_signal_start(handle, luv_signal_cb, signum);
  return luv_result(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 12, 0)
static int luv_signal_start_oneshot(lua_State* L) {
  uv_signal_t* handle = luv_check_signal(L, 1);
  int signum, ret;
  if (lua_isnumber(L, 2)) {
    signum = lua_tointeger(L, 2);
  }
  else if (lua_isstring(L, 2)) {
    signum = luv_sig_string_to_num(luaL_checkstring(L, 2));
    luaL_argcheck(L, signum, 2, "Invalid Signal name");
  }
  else {
    return luaL_argerror(L, 2, "Missing Signal name");
  }

  if (!lua_isnoneornil(L, 3)) {
    luv_check_callback(L, (luv_handle_t*)handle->data, LUV_SIGNAL, 3);
  }
  ret = uv_signal_start_oneshot(handle, luv_signal_cb, signum);
  return luv_result(L, ret);
}
#endif

static int luv_signal_stop(lua_State* L) {
  uv_signal_t* handle = luv_check_signal(L, 1);
  int ret = uv_signal_stop(handle);
  return luv_result(L, ret);
}
