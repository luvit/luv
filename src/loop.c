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

static int luv_loop_close(lua_State* L) {
  int ret = uv_loop_close(luv_loop(L));
  if (ret < 0) return luv_error(L, ret);
  luv_set_loop(L, NULL);
  lua_pushinteger(L, ret);
  return 1;
}

// These are the same order as uv_run_mode which also starts at 0
static const char *const luv_runmodes[] = {
  "default", "once", "nowait", NULL
};

static int luv_run(lua_State* L) {
  int mode = luaL_checkoption(L, 1, "default", luv_runmodes);
  luv_ctx_t* ctx = luv_context(L);
  ctx->mode = mode;
  int ret = uv_run(ctx->loop, (uv_run_mode)mode);
  ctx->mode = -1;
  if (ret < 0) return luv_error(L, ret);
  lua_pushboolean(L, ret);
  return 1;
}

static int luv_loop_mode(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);
  if (ctx->mode == -1) {
    lua_pushnil(L);
  } else {
    lua_pushstring(L, luv_runmodes[ctx->mode]);
  }
  return 1;
}

static int luv_loop_alive(lua_State* L) {
  int ret = uv_loop_alive(luv_loop(L));
  if (ret < 0) return luv_error(L, ret);
  lua_pushboolean(L, ret);
  return 1;
}

static int luv_stop(lua_State* L) {
  uv_stop(luv_loop(L));
  return 0;
}

static int luv_backend_fd(lua_State* L) {
  int ret = uv_backend_fd(luv_loop(L));
  // -1 is returned when there is no backend fd (like on Windows)
  if (ret == -1)
    lua_pushnil(L);
  else
    lua_pushinteger(L, ret);
  return 1;
}

static int luv_backend_timeout(lua_State* L) {
  int ret = uv_backend_timeout(luv_loop(L));
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_now(lua_State* L) {
  uint64_t now = uv_now(luv_loop(L));
  lua_pushinteger(L, now);
  return 1;
}

static int luv_update_time(lua_State* L) {
  uv_update_time(luv_loop(L));
  return 0;
}

static void luv_walk_cb(uv_handle_t* handle, void* arg) {
  lua_State* L = (lua_State*)arg;
  luv_handle_t* data = (luv_handle_t*)handle->data;

  // Sanity check
  // Most invalid values are large and refs are small, 0x1000000 is arbitrary.
  assert(data && data->ref < 0x1000000);

  lua_pushvalue(L, 1);           // Copy the function
  luv_find_handle(L, data);      // Get the userdata
  data->ctx->pcall(L, 1, 0, 0);  // Call the function
}

static int luv_walk(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);
  uv_walk(luv_loop(L), luv_walk_cb, L);
  return 0;
}

#if LUV_UV_VERSION_GEQ(1, 0, 2)
static const char *const luv_loop_configure_options[] = {
  "block_signal",
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  "metrics_idle_time",
#endif
  NULL
};

static int luv_loop_configure(lua_State* L) {
  uv_loop_t* loop = luv_loop(L);
  uv_loop_option option = 0;
  int ret = 0;
  switch (luaL_checkoption(L, 1, NULL, luv_loop_configure_options)) {
  case 0: option = UV_LOOP_BLOCK_SIGNAL; break;
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  case 1: option = UV_METRICS_IDLE_TIME; break;
#endif
  default: break; /* unreachable */
  }
  if (option == UV_LOOP_BLOCK_SIGNAL) {
    // lua_isstring checks for string or number
    int signal;
    luaL_argcheck(L, lua_isstring(L, 2), 2, "block_signal option: expected signal as string or number");
    signal = luv_parse_signal(L, 2);
    ret = uv_loop_configure(loop, UV_LOOP_BLOCK_SIGNAL, signal);
  } else {
    ret = uv_loop_configure(loop, option);
  }
  return luv_result(L, ret);
}
#endif
