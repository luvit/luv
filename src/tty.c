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

static int luv_tty_set_mode(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tty_t* handle = luv_get_tty(L, 1);
  int mode = luaL_checkint(L, 2);
  if (uv_tty_set_mode(handle, mode)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tty_set_mode: %s", uv_strerror(err));
  }
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top);
#endif
  return 0;
}

static int luv_tty_reset_mode(lua_State* L) {
  uv_tty_reset_mode();
  return 0;
}

static int luv_tty_get_winsize(lua_State* L) {
#ifdef LUV_STACK_CHECK
  int top = lua_gettop(L);
#endif
  uv_tty_t* handle = luv_get_tty(L, 1);
  int width, height;
  if(uv_tty_get_winsize(handle, &width, &height)) {
    uv_err_t err = uv_last_error(uv_default_loop());
    return luaL_error(L, "tty_get_winsize: %s", uv_strerror(err));
  }
  lua_pushinteger(L, width);
  lua_pushinteger(L, height);
#ifdef LUV_STACK_CHECK
  assert(lua_gettop(L) == top + 2);
#endif
  return 2;
}
