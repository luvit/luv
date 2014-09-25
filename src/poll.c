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

static int new_poll(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_poll_t* handle = lua_newuserdata(L, sizeof(*handle));
  int fd = luaL_checkinteger(L, 2);
  int ret;
  setup_udata(L, (uv_handle_t*)handle, "uv_handle");
  ret = uv_poll_init(loop, handle, fd);
  if (ret < 0) return luv_error(L, ret);
  return 1;
}

// These are the same order as uv_run_mode which also starts at 0
static const char *const pollevents[] = {
  "r", "w", "rw", NULL
};

static void poll_cb(uv_poll_t* handle, int status, int events) {
  lua_State* L = (lua_State*)handle->data;
  const char* evtstr;
  find_udata(L, handle);
  if (status < 0) {
    fprintf(stderr, "%s: %s\n", uv_err_name(status), uv_strerror(status));
    lua_pushstring(L, uv_err_name(status));
  }
  else {
    lua_pushnil(L);
  }
  switch (events) {
    case UV_READABLE: evtstr = "r"; break;
    case UV_WRITABLE: evtstr = "w"; break;
    case UV_READABLE|UV_WRITABLE: evtstr = "rw"; break;
    default: evtstr = ""; break;
  }
  lua_pushstring(L, evtstr);
  luv_emit_event(L, "onpoll", 3);
}

static int luv_poll_start(lua_State* L) {
  uv_poll_t* handle = luv_check_poll(L, 1);
  int events, ret;
  switch (luaL_checkoption(L, 2, "rw", pollevents)) {
    case 0: events = UV_READABLE; break;
    case 1: events = UV_WRITABLE; break;
    case 2: events = UV_READABLE | UV_WRITABLE; break;
  }
  ret = uv_poll_start(handle, events, poll_cb);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_poll_stop(lua_State* L) {
  uv_poll_t* handle = luv_check_poll(L, 1);
  int ret = uv_poll_stop(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}
