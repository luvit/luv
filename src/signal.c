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

static const char* signal_to_string(int signal) {
#ifdef SIGINT
  if (signal == SIGINT) return "SIGINT";
#endif
#ifdef SIGBREAK
  if (signal == SIGBREAK) return "SIGBREAK";
#endif
#ifdef SIGHUP
  if (signal == SIGHUP) return "SIGHUP";
#endif
#ifdef SIGWINCH
  if (signal == SIGWINCH) return "SIGWINCH";
#endif
  return "";
}

static int string_to_signal(const char* string) {
#ifdef SIGINT
  if (strcmp(string, "SIGINT") == 0) return SIGINT;
#endif
#ifdef SIGBREAK
  if (strcmp(string, "SIGBREAK") == 0) return SIGBREAK;
#endif
#ifdef SIGHUP
  if (strcmp(string, "SIGHUP") == 0) return SIGHUP;
#endif
#ifdef SIGWINCH
  if (strcmp(string, "SIGWINCH") == 0) return SIGWINCH;
#endif
  return 0;
}

static int new_signal(lua_State* L) {
  uv_loop_t* loop = luaL_checkudata(L, 1, "uv_loop");
  uv_signal_t* handle = lua_newuserdata(L, sizeof(*handle));
  int ret;
  setup_udata(L, handle, "uv_handle");
  ret = uv_signal_init(loop, handle);
  if (ret < 0) return luv_error(L, ret);
  return 1;
}

static void signal_cb(uv_signal_t* handle, int signum) {
  lua_State* L = (lua_State*)handle->data;
  find_udata(L, handle);
  lua_pushstring(L, signal_to_string(signum));
  luv_emit_event(L, "onsignal", 2);
}

static int luv_signal_start(lua_State* L) {
  uv_signal_t* handle = luv_check_signal(L, 1);
  int signum, ret;
  signum = string_to_signal(luaL_checkstring(L, 2));
  luaL_argcheck(L, signum, 2, "Invalid Signal name");
  handle->data = L;
  ret = uv_signal_start(handle, signal_cb, signum);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_signal_stop(lua_State* L) {
  uv_signal_t* handle = luv_check_signal(L, 1);
  int ret = uv_signal_stop(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}
