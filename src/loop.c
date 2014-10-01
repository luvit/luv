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

// These are the same order as uv_run_mode which also starts at 0
static const char *const runmodes[] = {
  "default", "once", "nowait", NULL
};

static int luv_loop_close(lua_State* L) {
  int ret = uv_loop_close(uv_default_loop());
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_run(lua_State* L) {
  int mode = luaL_checkoption(L, 1, "default", runmodes);
  int ret;
  // Record the lua state so callbacks can start here.
  R = L;
  ret = uv_run(uv_default_loop(), mode);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_loop_alive(lua_State* L) {
  int ret = uv_loop_alive(uv_default_loop());
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_stop(lua_State* L) {
  uv_stop(uv_default_loop());
  return 0;
}

static int luv_backend_fd(lua_State* L) {
  int ret = uv_backend_fd(uv_default_loop());
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_backend_timeout(lua_State* L) {
  int ret = uv_backend_timeout(uv_default_loop());
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_now(lua_State* L) {
  uint64_t now = uv_now(uv_default_loop());
  lua_pushinteger(L, now);
  return 1;
}

static int luv_update_time(lua_State* L) {
  uv_update_time(uv_default_loop());
  return 0;
}

static void walk_cb(uv_handle_t* handle, void* arg) {
  lua_State* L = arg;
  luv_handle_t* data = handle->data;

  // Sanity check
  // Most invalid values are large and refs are small, 0x1000000 is arbitrary.
  assert(data && data->ref < 0x1000000);

  lua_pushvalue(L, 1);      // Copy the function
  luv_find_handle(L, data); // Get the userdata
  lua_call(L, 1, 0);        // Call the function
}

static int luv_walk(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);
  uv_walk(uv_default_loop(), walk_cb, L);
  return 0;
}
