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

// uv.new_loop() -> loop
static int new_loop(lua_State* L) {
  // Allocate the structure in the lua vm
  uv_loop_t* loop = lua_newuserdata(L, sizeof(*loop));

  // Initialize and report any errors
  int ret = uv_loop_init(loop);
  if (ret < 0) return luv_error(L, ret);

  // Tag as "uv_loop" userdata
  luaL_getmetatable(L, "uv_loop");
  lua_setmetatable(L, -2);

  return 1;
}

// These are the same order as uv_run_mode which also starts at 0
static const char *const runmodes[] = {
  "default", "once", "nowait", NULL
};

static int luv_loop_close(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  int ret = uv_loop_close(loop);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

// uv.run(loop, mode)
static int luv_run(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  int mode = luaL_checkoption(L, 2, "default", runmodes);
  int ret = uv_run(loop, mode);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_loop_alive(lua_State* L) {
  const uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  int ret = uv_loop_alive(loop);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_stop(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_stop(loop);
  return 0;
}

static int luv_backend_fd(lua_State* L) {
  const uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  int ret = uv_backend_fd(loop);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_backend_timeout(lua_State* L) {
  const uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  int ret = uv_backend_timeout(loop);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_now(lua_State* L) {
  const uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uint64_t now = uv_now(loop);
  lua_pushinteger(L, now);
  return 1;
}

static int luv_update_time(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_update_time(loop);
  return 0;
}

static void walk_cb(uv_handle_t* handle, void* arg) {
  lua_State* L = arg;
  // TODO: send something more useful to lua side.
  printf("walk L=%p handle=%p\n", L, handle);
  lua_pushvalue(L, 2); // Copy the lua callback
  lua_pushlightuserdata(L, handle); 
  lua_call(L, 1,0);
}

static int luv_walk(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  luaL_checktype(L, 2, LUA_TFUNCTION);
  uv_walk(loop, walk_cb, L);
  return 0;
}
