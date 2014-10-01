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

static int luv_new_fs_event(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_event_t* handle = luv_create_fs_event(L);
  int ret = uv_fs_event_init(loop, handle);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static void luv_fs_event_cb(uv_fs_event_t* handle, const char* filename, int events, int status) {
  // self
  lua_State* L = luv_find(handle->data);

  // err
  if (status < 0) {
    fprintf(stderr, "%s: %s\n", uv_err_name(status), uv_strerror(status));
    lua_pushstring(L, uv_err_name(status));
  }
  else {
    lua_pushnil(L);
  }

  // filename
  lua_pushstring(L, filename);

  // events
  lua_newtable(L);
  if (events & UV_FS_EVENT_WATCH_ENTRY) {
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "watch_entry");
  }
  if (events & UV_FS_EVENT_STAT) {
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "stat");
  }
  if (events & UV_FS_EVENT_RECURSIVE) {
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "recursive");
  }

  luv_emit_event(L, handle->data, "onevent", 4);
}

static int luv_fs_event_start(lua_State* L) {
  uv_fs_event_t* handle = luv_check_fs_event(L, 1);
  const char* path = luaL_checkstring(L, 2);
  int flags = 0, ret;
  luaL_checktype(L, 3, LUA_TTABLE);
  lua_getfield(L, 3, "watch_entry");
  if (lua_toboolean(L, -1)) flags |= UV_FS_EVENT_WATCH_ENTRY;
  lua_pop(L, 1);
  lua_getfield(L, 3, "stat");
  if (lua_toboolean(L, -1)) flags |= UV_FS_EVENT_STAT;
  lua_pop(L, 1);
  lua_getfield(L, 3, "recursive");
  if (lua_toboolean(L, -1)) flags |= UV_FS_EVENT_RECURSIVE;
  lua_pop(L, 1);
  luv_ref_state(handle->data, L);
  ret = uv_fs_event_start(handle, luv_fs_event_cb, path, flags);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_fs_event_stop(lua_State* L) {
  uv_fs_event_t* handle = luv_check_fs_event(L, 1);
  int ret = uv_fs_event_stop(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_fs_event_getpath(lua_State* L) {
  uv_fs_event_t* handle = luv_check_fs_event(L, 1);
  size_t len = 2*PATH_MAX;
  char buf[2*PATH_MAX];
  int ret = uv_fs_event_getpath(handle, buf, &len);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, buf, len);
  return 1;
}
