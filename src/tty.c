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

static int new_tty(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  int readable, ret;
  uv_tty_t* handle;
  uv_file fd = luaL_checkinteger(L, 2);
  luaL_checktype(L, 3, LUA_TBOOLEAN);
  readable = lua_toboolean(L, 3);
  handle = luv_create_tty(L);
  ret = uv_tty_init(loop, handle, fd, readable);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_tty_set_mode(lua_State* L) {
  uv_tty_t* handle = luv_check_tty(L, 1);
  int mode = luaL_checkinteger(L, 2);
  int ret = uv_tty_set_mode(handle, mode);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_tty_reset_mode(__attribute__((unused)) lua_State* L) {
  int ret = uv_tty_reset_mode();
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_tty_get_winsize(lua_State* L) {
  uv_tty_t* handle = luv_check_tty(L, 1);
  int width, height;
  int ret = uv_tty_get_winsize(handle, &width, &height);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, width);
  lua_pushinteger(L, height);
  return 2;
}
