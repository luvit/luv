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

static int new_fs_poll(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_fs_poll_t* handle = luv_create_fs_poll(L);
  int ret = uv_fs_poll_init(loop, handle);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static void luv_push_timespec_table(lua_State* L, const uv_timespec_t* t) {
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, t->tv_sec);
  lua_setfield(L, -2, "sec");
  lua_pushinteger(L, t->tv_nsec);
  lua_setfield(L, -2, "nsec");
}

static void luv_push_stats_table(lua_State* L, const uv_stat_t* s) {
  lua_createtable(L, 0, 16);
  lua_pushinteger(L, s->st_dev);
  lua_setfield(L, -2, "dev");
  lua_pushinteger(L, s->st_mode);
  lua_setfield(L, -2, "mode");
  lua_pushinteger(L, s->st_nlink);
  lua_setfield(L, -2, "nlink");
  lua_pushinteger(L, s->st_uid);
  lua_setfield(L, -2, "uid");
  lua_pushinteger(L, s->st_gid);
  lua_setfield(L, -2, "gid");
  lua_pushinteger(L, s->st_rdev);
  lua_setfield(L, -2, "rdev");
  lua_pushinteger(L, s->st_ino);
  lua_setfield(L, -2, "ino");
  lua_pushinteger(L, s->st_size);
  lua_setfield(L, -2, "size");
  lua_pushinteger(L, s->st_blksize);
  lua_setfield(L, -2, "blksize");
  lua_pushinteger(L, s->st_blocks);
  lua_setfield(L, -2, "blocks");
  lua_pushinteger(L, s->st_flags);
  lua_setfield(L, -2, "flags");
  lua_pushinteger(L, s->st_gen);
  lua_setfield(L, -2, "gen");
  luv_push_timespec_table(L, &s->st_atim);
  lua_setfield(L, -2, "atim");
  luv_push_timespec_table(L, &s->st_mtim);
  lua_setfield(L, -2, "mtim");
  luv_push_timespec_table(L, &s->st_ctim);
  lua_setfield(L, -2, "ctim");
  luv_push_timespec_table(L, &s->st_birthtim);
  lua_setfield(L, -2, "birthtim");
}

static void fs_poll_cb(uv_fs_poll_t* handle, int status, const uv_stat_t* prev, const uv_stat_t* curr) {
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

  // prev
  if (prev) {
    luv_push_stats_table(L, prev);
  }
  else {
    lua_pushnil(L);
  }

  // curr
  if (curr) {
    luv_push_stats_table(L, curr);
  }
  else {
    lua_pushnil(L);
  }

  luv_emit_event(L, handle->data, "onpoll", 4);
}

static int luv_fs_poll_start(lua_State* L) {
  uv_fs_poll_t* handle = luv_check_fs_poll(L, 1);
  const char* path = luaL_checkstring(L, 2);
  unsigned int interval = luaL_checkinteger(L, 3);
  int ret;
  luv_ref_state(handle->data, L);
  ret = uv_fs_poll_start(handle, fs_poll_cb, path, interval);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_fs_poll_stop(lua_State* L) {
  uv_fs_poll_t* handle = luv_check_fs_poll(L, 1);
  int ret = uv_fs_poll_stop(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_fs_poll_getpath(lua_State* L) {
  uv_fs_poll_t* handle = luv_check_fs_poll(L, 1);
  size_t len = 2*PATH_MAX;
  char buf[2*PATH_MAX];
  int ret = uv_fs_poll_getpath(handle, buf, &len);
  if (ret < 0) return luv_error(L, ret);
  lua_pushlstring(L, buf, len);
  return 1;
}
