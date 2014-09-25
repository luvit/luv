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

static int new_idle(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_idle_t* handle = lua_newuserdata(L, sizeof(*handle));
  int ret;
  setup_udata(L, handle, "uv_handle");
  ret = uv_idle_init(loop, handle);
  if (ret < 0) return luv_error(L, ret);
  return 1;
}

static void idle_cb(uv_idle_t* handle) {
  lua_State* L = (lua_State*)handle->data;
  find_udata(L, handle);
  luv_emit_event(L, "onidle", 1);
}

static int luv_idle_start(lua_State* L) {
  uv_idle_t* handle = luv_check_idle(L, 1);
  int ret;
  handle->data = L;
  ret = uv_idle_start(handle, idle_cb);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_idle_stop(lua_State* L) {
  uv_idle_t* handle = luv_check_idle(L, 1);
  int ret = uv_idle_stop(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

