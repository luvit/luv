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

static int loop_tostring(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  lua_pushfstring(L, "uv_loop_t: %p", loop);
  return 1;
}

static void loop_init(lua_State* L) {
  luaL_newmetatable (L, "uv_loop");
  lua_pushcfunction(L, loop_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pop(L, 1);
}

// uv.new_loop() -> loop
static int new_loop(lua_State* L) {
  uv_loop_t* loop = luv_create_loop(L);
  int ret = uv_loop_init(loop);
  if (ret < 0) return luv_error(L, ret);
  return 1;
}

// These are the same order as uv_run_mode which also starts at 0
static const char *const runmodes[] = {
  "default", "once", "nowait", NULL
};

static int luv_loop_close(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  int ret = uv_loop_close(loop);
  if (ret < 0) return luv_error(L, ret);
  luv_unref_loop(loop);
  lua_pushinteger(L, ret);
  return 1;
}

// uv.run(loop, mode)
static int luv_run(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  int mode = luaL_checkoption(L, 2, "default", runmodes);
  int ret = uv_run(loop, mode);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_loop_alive(lua_State* L) {
  const uv_loop_t* loop = luv_check_loop(L, 1);
  int ret = uv_loop_alive(loop);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_stop(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_stop(loop);
  return 0;
}

static int luv_backend_fd(lua_State* L) {
  const uv_loop_t* loop = luv_check_loop(L, 1);
  int ret = uv_backend_fd(loop);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_backend_timeout(lua_State* L) {
  const uv_loop_t* loop = luv_check_loop(L, 1);
  int ret = uv_backend_timeout(loop);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_now(lua_State* L) {
  const uv_loop_t* loop = luv_check_loop(L, 1);
  uint64_t now = uv_now(loop);
  lua_pushinteger(L, now);
  return 1;
}

static int luv_update_time(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_update_time(loop);
  return 0;
}

static void walk_cb(uv_handle_t* handle, void* arg) {
  lua_State* L = arg;
  luv_find_handle(handle); // Look up the userdata for this handle
  lua_rawseti(L, -2, lua_rawlen(L, -2) + 1);
}

static int luv_walk(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  lua_newtable(L);
  uv_walk(loop, walk_cb, L);
  return 1;
}
