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

static uv_tty_t* luv_check_tty(lua_State* L, int index) {
  uv_tty_t* handle = (uv_tty_t*)luv_checkudata(L, index, "uv_tty");
  luaL_argcheck(L, handle->type == UV_TTY && handle->data, index, "Expected uv_tty_t");
  return handle;
}

static int luv_new_tty(lua_State* L) {
  int readable, ret;
  uv_tty_t* handle;
  luv_ctx_t* ctx = luv_context(L);
  uv_file fd = luaL_checkinteger(L, 1);
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  readable = lua_toboolean(L, 2);
  handle = (uv_tty_t*)luv_newuserdata(L, sizeof(*handle));
  ret = uv_tty_init(ctx->loop, handle, fd, readable);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  handle->data = luv_setup_handle(L, ctx);
  return 1;
}

static int luv_tty_set_mode(lua_State* L) {
  uv_tty_t* handle = luv_check_tty(L, 1);
  int mode = luaL_checkinteger(L, 2);
  int ret = uv_tty_set_mode(handle, mode);
  return luv_result(L, ret);
}

static int luv_tty_reset_mode(lua_State* L) {
  int ret = uv_tty_reset_mode();
  return luv_result(L, ret);
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

#if LUV_UV_VERSION_GEQ(1, 33, 0)
int luv_tty_set_vterm_state(lua_State* L)
{
  uv_tty_vtermstate_t state[] = {UV_TTY_SUPPORTED, UV_TTY_UNSUPPORTED};
  const char* option[] = {"supported", "unsupported", NULL};
  int idx = luaL_checkoption (L, 1, NULL, option);
  uv_tty_set_vterm_state(state[idx]);
  return 0;
}

int luv_tty_get_vterm_state(lua_State* L)
{
  uv_tty_vtermstate_t state;
  int ret = uv_tty_get_vterm_state(&state);
  if (ret < 0) return luv_error(L, ret);
  switch (state)
  {
    case UV_TTY_SUPPORTED:
      lua_pushliteral(L, "supported");
      ret = 1;
      break;
    case UV_TTY_UNSUPPORTED:
      lua_pushliteral(L, "unsupported");
      ret = 1;
      break;
    default:
      ret = luaL_error(L, "unexpected uv_tty_vtermstate: %d", state);
  }
  return ret;
}

#endif

